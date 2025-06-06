// Hamster thread

#pragma once

#include <memory/stl_sequential.hpp>
#include <functional>
#include <cstddef>
#include <cstdint>

namespace Hamster
{
    enum class ThreadState : uint8_t
    {
        RUNNING,
        ENDED,
    };

    class Process;

    class Thread
    {
    public:
        Thread(Process *process, size_t id);
        ~Thread() = default;
        Thread(Thread &&);
        Thread &operator=(Thread &&);

        Thread(const Thread &) = default;
        Thread &operator=(const Thread &) = default;

        void set_state(ThreadState state) { this->state = state; }
        ThreadState get_state() const { return state; }
        Process *get_process() const { return process; }
        void set_process(Process *process) { this->process = process; }
        size_t get_id() const { return id; }
        void set_pc(uint32_t pc) { this->pc = pc; }
        uint64_t get_tick_count() const { return tick_count; }

        uint32_t *get_regs() { return x; }
        double *get_fregs() { return f; }

        int get_error_code() const { return error_code; }
        void set_error_code(int code) { error_code = code; }

        int get_signal_mask() const { return signal_mask; }
        void set_signal_mask(int mask) { signal_mask = mask; }

        int get_pending_signal() const { return pending_signal; }

        /**
         * @brief Tick the thread once
         * @note This executes one instruction
         */
        void tick();

        /**
         * @brief Pause the thread
         * @param callback Callback that will decide when to resume the thread
         * @note The callback will be given this thread as an argument
         * @note If the thread is already paused, the previous callback will be pushed down a stack,
         *     * and instead of resuming when this callback is done, it will continue pausing with the previous
         *     * callback. Note that to unpause from the callback, you call `resume()` on the thread.
         */
        void pause(const std::function<void(Thread &)> &callback);

        /**
         * @brief Get the current pause callback, if any
         * @return The current pause callback, or nullptr if there is none
         * @note This will not remove the callback, just return it
         */
        const std::function<void(Thread &)> &get_current_pause_callback() const;

        /**
         * @brief Check if the thread is paused
         * @return True if there is at least one pause callback
         */
        bool is_paused() const { return !pause_callbacks.empty(); }

        /**
         * @brief Resume the thread
         * @note This will not check with the callback, but will forcibly resume
         */
        void resume();

        /**
         * @brief Send a signal to the thread
         * @param signal Signal to send
         * @note The handling of the signal is deferred to the next tick
         */
        void signal(int signal);

    private:
        ThreadState state;
        Process *process;
        Deque<std::function<void(Thread &)>> pause_callbacks;
        size_t id;

        uint32_t x[32];
        double f[32];
        uint32_t fcsr;
        uint32_t pc;

        int pending_signal;
        int signal_mask;

        int error_code;

        // Minimum timing implementation
        uint64_t tick_count;

        int read32(uint32_t addr, uint32_t &out);
        int read16(uint32_t addr, uint16_t &out);
        int read8(uint32_t addr, uint8_t &out);
        int write32(uint32_t addr, uint32_t value);
        int write16(uint32_t addr, uint16_t value);
        int write8(uint32_t addr, uint8_t value);

        int readf32(uint32_t addr, float &out);
        int readf64(uint32_t addr, double &out);
        int writef32(uint32_t addr, float value);
        int writef64(uint32_t addr, double value);

        int execute(uint32_t inst);

        void handle_signal();
    };
} // namespace Hamster

