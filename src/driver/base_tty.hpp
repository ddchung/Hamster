// TTY driver that provides terminal functionality, while wrapping
// around a user-defined "backend"

#pragma once

#include <filesystem/base_file.hpp>
#include <filesystem/device_manager.hpp>
#include <memory/allocator.hpp>
#include <memory/stl_sequential.hpp>
#include <kscheduler/kscheduler.hpp>
#include <process/task.hpp>
#include <abi/values.hpp>
#include <abi/structs.hpp>
#include <errno/errno.h>
#include <utility>

namespace Hamster
{
    /*
    Backend spec:


    // Note: The exact signatures don't matter, as long as they can be called
    //       as-if they are as specified here. This enables the use of proxy
    //       objects and other patterns

    struct
    {
        ssize_t read(void *buf, size_t count);
        ssize_t write(const void *buf, size_t count);

        int get_win_sz(sys_winsize *ws);
    };
    */

    template <typename Backend>
    class BaseTTYDriver;

    template <typename Backend>
    class BaseTTYHandle : public BaseCharacterDeviceHandle
    {
    public:
        BaseTTYHandle(class BaseTTYDriver<Backend> *driver, int flags)
            : driver(driver), flags(flags) {}

        ~BaseTTYHandle() override = default;

        ssize_t write(const uint8_t *buf, size_t size) override;
        ssize_t read(uint8_t *buf, size_t size) override;
        int get_flags() override { return flags; }
        int set_flags(int flags) override
        {
            this->flags = flags;
            return 0;
        }
        int ioctl(int request, IoctlArg arg = {}) override;
        bool is_tty() override { return true; }

        int64_t seek(int64_t offset, int whence) override
        {
            error = H_ESPIPE; // TTYs do not support seeking
            return -1;
        }
        int64_t tell() override
        {
            error = H_ESPIPE;
            return -1;
        }
        int poll(int op) override;

    private:
        class BaseTTYDriver<Backend> *driver;
        int flags;
    };

    template <typename Backend>
    class BaseTTYDriver : public Backend, public BaseSpecialDriver
    {
    public:
        template <typename... Args>
        BaseTTYDriver(Args &&...args)
            : Backend(std::forward<Args>(args)...)
        {
            // Get window size
            get_win_sz(&win_sz);

            // Initialize termios with default values

            // Input flags (ICRNL enables carriage return -> newline translation)
            termios.iflag = H_ICRNL | H_IXON;

            // Output flags (OPOST enables post-processing of output)
            termios.oflag = H_OPOST | H_ONLCR;

            // Control flags (CS8 for 8-bit characters, CREAD to enable receiver)
            termios.cflag = H_CREAD | H_CS8;

            // Local flags
            termios.lflag = H_ISIG | H_ICANON | H_ECHO | H_ECHOE | H_ECHOK;

            // Control characters
            termios.cc[H_VINTR] = 0x03;  // Ctrl+C
            termios.cc[H_VQUIT] = 0x1C;  // Ctrl+backslash
            termios.cc[H_VERASE] = 0x7F; // DEL
            termios.cc[H_VKILL] = 0x15;  // Ctrl+U
            termios.cc[H_VEOF] = 0x04;   // Ctrl+D
            termios.cc[H_VEOL] = 0;      // Not used
            termios.cc[H_VSTART] = 0x11; // Ctrl+Q
            termios.cc[H_VSTOP] = 0x13;  // Ctrl+S
            termios.cc[H_VSUSP] = 0x1A;  // Ctrl+Z

            // Create a kernel task to poll input
            class TTYInputPollTask : public BaseKTask
            {
            public:
                TTYInputPollTask(BaseTTYDriver *driver)
                    : driver(driver) 
                {
                    id = (uint32_t)(uintptr_t)driver;
                    flags = KSCHED_AUTO_INTERVAL;
                    interval = 100; // Poll every 100ms
                }

                void run() override
                {
                    driver->read_all_pending();
                    driver->check_winsize();
                }
            private:
                BaseTTYDriver *driver;
            };

            auto *task = alloc<TTYInputPollTask>(1, this);
            kscheduler.add_task(task);
        }

        virtual ~BaseTTYDriver()
        {
            // Remove the input poll task
            kscheduler.remove_task((uint32_t)(uintptr_t)this);
        }

        BaseSpecialDriverHandle *create_handle(int flags) override
        {
            return alloc<BaseTTYHandle<Backend>>(1, this, flags);
        }

        // Check if the calling process is in the foreground and can read
        // If not, send SIGTTIN to the process group
        // Note that if the calling process ignores/blocks SIGTTIN, it is still allowed to read
        //
        // Returns: 0 if allowed, 1 if not allowed, -1 on error
        int check_readable()
        {
            Task *current_task = Task::get_current_task();
            if (!current_task)
                return 0; // Always allow kernel to read

            if (current_task->get_sid() != sid)
            {
                error = H_ENOTTY;
                return -1;
            }

            if (fg_pgid && current_task->get_pgid() != fg_pgid &&
                !current_task->is_signal_blocked(H_SIGTTIN) && 
                !current_task->is_signal_ignored(H_SIGTTIN))
            {
                current_task->send_signal_process(make_kill_siginfo(H_SIGTTIN));
            }
            return 0;
        }

        // Same thing as check_readable, but for writing
        // Returns: 0 if allowed, 1 if not allowed, -1 on error
        int check_writable()
        {
            Task *current_task = Task::get_current_task();
            if (!current_task)
                return 0; // Always allow kernel to write

            if (current_task->get_sid() != sid)
            {
                error = H_ENOTTY;
                return -1;
            }

            if (fg_pgid && current_task->get_pgid() != fg_pgid &&
                !current_task->is_signal_blocked(H_SIGTTOU) && 
                !current_task->is_signal_ignored(H_SIGTTOU))
            {
                current_task->send_signal_process(make_kill_siginfo(H_SIGTTOU));
            }
            return 0;
        }

        // Handle a single special character
        // This can do things such as sending SIGINT to the foreground process group on CTRL+C char
        // Returns: 0 on success, -1 on error, 1 if not special
        int handle_special_char(char c)
        {
            if (c == termios.cc[H_VINTR] && (termios.lflag & H_ISIG))
            {
                return send_sig_to_fg(H_SIGINT);
            }
            else if (c == termios.cc[H_VQUIT] && (termios.lflag & H_ISIG))
            {
                return send_sig_to_fg(H_SIGQUIT);
            }
            else if (c == termios.cc[H_VSUSP] && (termios.lflag & H_ISIG))
            {
                return send_sig_to_fg(H_SIGTSTP);
            }
            else if (c == termios.cc[H_VSTART] && (termios.iflag & H_IXON))
            {
                output_stopped = false; // Resume output
                return 0;
            }
            else if (c == termios.cc[H_VSTOP] && (termios.iflag & H_IXON))
            {
                output_stopped = true; // Stop output
                return 0;
            }
            else if (c == termios.cc[H_VERASE] && (termios.lflag & H_ICANON))
            {
                if (!input_buffer.empty())
                {
                    input_buffer.pop_back();
                    if (termios.lflag & H_ECHO)
                        write("\b \b", 3); // Clear last character
                }
                return 0;
            }

            return 1;
        }

        // Send a signal to the foreground process group
        int send_sig_to_fg(int signo)
        {
            if (!fg_pgid)
                return -1;
            Task *task = Task::get_task_pgid(fg_pgid);
            
            return task->send_signal_pgroup(make_kill_siginfo(signo));
        }

        int flush_output()
        {
            for (char c : output_buffer)
            {
                if ((termios.oflag & H_OPOST))
                {
                    if (c == '\n' && (termios.oflag & H_ONLCR))
                    {
                        // Properly expand newline to CRLF
                        if (write("\r\n", 2) < 0)
                            return -1;
                        continue;
                    }
                    else if (c == '\r' && (termios.oflag & H_OCRNL))
                    {
                        c = '\n'; // CR becomes NL
                    }
                    else if (c == '\n' && (termios.oflag & H_ONLRET))
                    {
                        c = '\r'; // NL becomes CR
                    }
                }

                if (write(&c, 1) < 0)
                    return -1;
            }
            output_buffer.clear();
            return 0;
        }

        int flush_input()
        {
            input_buffer.clear();
            return 0;
        }

        void read_all_pending()
        {
            static uint8_t buf[1024];
            ssize_t bytes_read = read(buf, sizeof(buf));
            if (bytes_read < 0)
                return;

            // Append to input buffer
            for (ssize_t i = 0; i < bytes_read; ++i)
            {
                char c = buf[i];
                char original_c = c;

                // Input translation (POSIX input processing)
                if (c == '\n' && (termios.iflag & H_INLCR))
                    c = '\r';
                if (c == '\r' && (termios.iflag & H_IGNCR))
                    continue;
                if (c == '\r' && (termios.iflag & H_ICRNL))
                    c = '\n';
                
                // Echo handling
                if (termios.lflag & H_ECHO && !(original_c == termios.cc[H_VEOF] ||
                        original_c == termios.cc[H_VEOL] ||
                        original_c == termios.cc[H_VERASE]))
                {
                    if (original_c == '\r' || original_c == '\n')
                    {
                        // Echo CR+LF for newline (mimics Linux behavior)
                        output_buffer.push_back('\r');
                        output_buffer.push_back('\n');
                    }
                    else if ((original_c & 0x1F) == original_c) // Control characters
                    {
                        output_buffer.push_back('^');
                        output_buffer.push_back(original_c + 'A' - 1); // CTRL+A → ^A, etc.
                    }
                    else if (original_c == 0x7F) // DEL
                    {
                        output_buffer.push_back('^');
                        output_buffer.push_back('?');
                    }
                    else
                    {
                        output_buffer.push_back(original_c);
                    }

                    flush_output();
                }

                // Handle special characters (e.g., VEOF, VERASE, etc.)
                int special_result = handle_special_char(original_c);
                if (special_result != 1)
                    continue;

                // Add to canonical input buffer
                input_buffer.push_back(c);
            }
        }

        void check_winsize()
        {
            // Check if the window size has changed
            sys_winsize new_size;
            if (get_win_sz(&new_size) < 0)
                return;

            if (new_size.col != win_sz.col || new_size.row != win_sz.row ||
                new_size.xpixel != win_sz.xpixel || new_size.ypixel != win_sz.ypixel)
            {
                win_sz = new_size;
                // Send SIGWINCH to the foreground process group
                send_sig_to_fg(H_SIGWINCH);
            }

            // Update the terminal size
            win_sz = new_size;
        }

        // Ensure they are accessible

        using Backend::get_win_sz;
        using Backend::read;
        using Backend::write;

        // Session *session = nullptr;
        // ProcessGroup *fg_pgroup = nullptr;
        uint32_t sid = 0;
        uint32_t fg_pgid = 0;
        sys_termios termios = {};
        sys_winsize win_sz = {};

        Deque<char> input_buffer;
        Deque<char> output_buffer;
        bool output_stopped : 1 = false;
    };

    template <typename Backend>
    ssize_t BaseTTYHandle<Backend>::read(uint8_t *buf, size_t size)
    {
        if (driver->check_readable() < 0)
            return -1; // Not allowed to read


        // Do buffering
        // Canonical: line buffering
        // Non-canonical: cc[VMIN] and cc[VTIME] control the buffering

        // Start with -1: no characters to read
        ssize_t to_read = -1;
        if (driver->termios.lflag & H_ICANON)
        {
            // Read line by line
            for (ssize_t i = 0; i < (ssize_t)driver->input_buffer.size(); ++i)
            {
                char c = driver->input_buffer[i];
                if (i == 0 && c == driver->termios.cc[H_VEOF])
                {
                    // EOF character, return 0
                    driver->input_buffer.pop_front();
                    return 0;
                }
                if (c == '\n' || c == driver->termios.cc[H_VEOL] || c == driver->termios.cc[H_VEOF])
                {
                    // complete line
                    to_read = i + 1; // Read up to and including this character
                    break;
                }
            }
        }
        else
        {
            // Non-canonical mode
            // Note that we won't implement VTIME for now

            if (driver->input_buffer.size() >= driver->termios.cc[H_VMIN])
            {
                to_read = std::min<size_t>(driver->input_buffer.size(), size);
            }
        }

        if (to_read == -1)
        {
            error = H_EAGAIN;
            return -1;
        }

        if (to_read > (ssize_t)size)
        {
            to_read = size;
        }

        for (ssize_t i = 0; i < to_read && i < (ssize_t)size; ++i)
        {
            buf[i] = driver->input_buffer[i];
        }
        driver->input_buffer.erase(driver->input_buffer.begin(), driver->input_buffer.begin() + to_read);

        return to_read;
    }

    template <typename Backend>
    ssize_t BaseTTYHandle<Backend>::write(const uint8_t *buf, size_t size)
    {
        if (driver->termios.lflag & H_TOSTOP)
        {
            if (driver->check_writable() < 0)
                return -1; // Not allowed to write
        }

        // Check if output is stopped and flow control is enabled
        if (driver->output_stopped && (driver->termios.iflag & H_IXON))
        {
            error = H_EAGAIN;
            return -1;
        }

        // Write to the output buffer
        for (size_t i = 0; i < size; ++i)
        {
            driver->output_buffer.push_back(buf[i]);
        }

        if (driver->flush_output() < 0)
        {
            return -1; // Error while flushing output
        }

        return size;
    }

    template <typename Backend>
    int BaseTTYHandle<Backend>::poll(int op)
    {
        if (op & POLL_READ)
        {
            if (driver->termios.lflag & H_ICANON)
            {
                // Canonical mode: check if we have a complete line
                bool has_line = false;
                for (char c : driver->input_buffer)
                {
                    if (c == '\n' || c == driver->termios.cc[H_VEOL] || c == driver->termios.cc[H_VEOF])
                    {
                        has_line = true;
                        break;
                    }
                }
                if (!has_line)
                    return 0; // Not ready for reading
            }
            else
            {
                // Non-canonical mode, check if we have enough characters
                if (driver->input_buffer.size() < driver->termios.cc[H_VMIN])
                {
                    return 0;
                }
            }
        }
        if (op & POLL_WRITE)
        {
            // Check if we can write
            if (driver->output_stopped && (driver->termios.iflag & H_IXON))
            {
                return 0; // Not ready for writing
            }
        }

        return 1; // Ready for both
    }

    template <typename Backend>
    int BaseTTYHandle<Backend>::ioctl(int request, IoctlArg arg)
    {
        switch (request)
        {
        case H_TCGETS:
            *((sys_termios *)arg.p) = driver->termios;
            return 0;
        case H_TCSETS:
            driver->termios = *(const sys_termios *)arg.p;
            return 0;
        case H_TCSETSW:
            if (driver->flush_output() < 0)
                return -1;
            driver->termios = *(const sys_termios *)arg.p;
            return 0;
        case H_TCSETSF:
            if (driver->flush_input() < 0)
                return -1;
            if (driver->flush_output() < 0)
                return -1;
            driver->termios = *(const sys_termios *)arg.p;
            return 0;
        case H_TIOCGWINSZ:
            *(sys_winsize *)arg.p = driver->win_sz;
            return 0;
        case H_TIOCGPGRP:
            // *(uint32_t *)arg.p = driver->fg_pgroup ? driver->fg_pgroup->pgid : 0;
            return 0;
        case H_TIOCSCTTY:
        {
            // // Set the controlling TTY
            if (driver->sid && arg.i == 0)
            {
                error = H_EPERM;
                return -1;
            }
            Task *current_task = Task::get_current_task();
            if (!current_task)
            {
                error = H_EINVAL;
                return -1; // No current task
            }
            if (driver->sid)
            {
                // Stealing not supported
                error = H_ENOTSUP;
                return -1;
            }
            driver->sid = current_task->get_sid();
            return 0;
        }
        case H_TIOCSPGRP:
        {
            uint32_t pgid = *(uint32_t *)arg.p;
            Task *task = Task::get_task_pgid(pgid);
            if (!task)
            {
                error = H_EPERM;
                return -1;
            }
            if (task->get_sid() != driver->sid)
            {
                error = H_EPERM;
                return -1;
            }
            driver->fg_pgid = task->get_pgid();

            return 0;
        }
        case H_TIOCSWINSZ:
        default:
            error = H_ENOSYS;
            return -1;
        }
    }
} // namespace Hamster
