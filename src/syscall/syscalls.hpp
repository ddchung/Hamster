// Hamster system calls
// see: https://www.chromium.org/chromium-os/developer-library/reference/linux-constants/syscalls/#x86_64-64-bit

#pragma once

#include <cstdint>

// put syscalls in a separate namespace
// there are a lot of them
namespace Hamster
{
    namespace Syscalls
    {
        uint32_t exit(uint32_t status);
        uint32_t log(uint32_t message_addr);
    } // namespace Syscalls
    

    /* not used yet
    // didn't use namespace to support both 32- and 64-bit
    // syscall declarations
    template <typename word_t>
    class Syscalls
    {
    public:
        static word_t read(word_t fd, word_t buf, word_t count, word_t, word_t, word_t);
        static word_t write(word_t fd, word_t buf, word_t count, word_t, word_t, word_t);
        static word_t open(word_t filename, word_t flags, word_t mode, word_t, word_t, word_t);
        static word_t close(word_t fd, word_t, word_t, word_t, word_t, word_t);
        static word_t stat(word_t filename, word_t buf, word_t, word_t, word_t, word_t);
        static word_t fstat(word_t fd, word_t buf, word_t, word_t, word_t, word_t);
        static word_t lstat(word_t filename, word_t buf, word_t, word_t, word_t, word_t);
        static word_t poll(word_t fds, word_t nfds, word_t timeout, word_t, word_t, word_t);
        static word_t lseek(word_t fd, word_t offset, word_t whence, word_t, word_t, word_t);
        static word_t mmap(word_t, word_t, word_t, word_t, word_t, word_t); //< unkown parameter names
        static word_t mprotect(word_t start, word_t len, word_t prot, word_t, word_t, word_t);
        static word_t munmap(word_t start, word_t len, word_t, word_t, word_t, word_t);
        static word_t brk(word_t brk, word_t, word_t, word_t, word_t, word_t);
        static word_t rt_sigaction(word_t signum, word_t act, word_t oldact, word_t, word_t, word_t);
        static word_t rt_sigprocmask(word_t how, word_t set, word_t oldset, word_t, word_t, word_t);
        static word_t rt_sigreturn(word_t, word_t, word_t, word_t, word_t, word_t); // < unkown parameter names
        static word_t ioctl(word_t fd, word_t request, word_t argp, word_t, word_t, word_t);
        static word_t pread64(word_t fd, word_t buf, word_t count, word_t offset_high, word_t offset_low, word_t);
        static word_t pwrite64(word_t fd, word_t buf, word_t count, word_t offset_high, word_t offset_low, word_t);
        static word_t readv(word_t fd, word_t iov, word_t count, word_t, word_t, word_t);
        static word_t writev(word_t fd, word_t iov, word_t count, word_t, word_t, word_t);
        static word_t access(word_t filename, word_t mode, word_t, word_t, word_t, word_t);
        static word_t pipe(word_t fds, word_t, word_t, word_t, word_t, word_t);
        static word_t select(word_t n, word_t readfds, word_t writefds, word_t exceptfds, word_t timeout, word_t);
        static word_t sched_yield(word_t, word_t, word_t, word_t, word_t, word_t);
        static word_t mremap(word_t old_address, word_t old_size, word_t new_size, word_t flags, word_t new_address, word_t);
        static word_t msync(word_t start, word_t len, word_t flags, word_t, word_t, word_t);
        static word_t mincore(word_t start, word_t len, word_t vec, word_t, word_t, word_t);
        static word_t madvise(word_t start, word_t len, word_t advice, word_t, word_t, word_t);
        static word_t shmget(word_t key, word_t size, word_t shmflg, word_t, word_t, word_t);
        static word_t shmat(word_t shmid, word_t shmaddr, word_t shmflg, word_t, word_t, word_t);
        static word_t shmctl(word_t shmid, word_t cmd, word_t buf, word_t, word_t, word_t);
        static word_t dup(word_t oldfd, word_t, word_t, word_t, word_t, word_t);
        static word_t dup2(word_t oldfd, word_t newfd, word_t, word_t, word_t, word_t);
        static word_t pause(word_t, word_t, word_t, word_t, word_t, word_t);
        static word_t nanosleep(word_t req, word_t rem, word_t, word_t, word_t, word_t);
        static word_t getitimer(word_t which, word_t curr_value, word_t, word_t, word_t, word_t);
        static word_t alarm(word_t seconds, word_t, word_t, word_t, word_t, word_t);
        static word_t setitimer(word_t which, word_t new_value, word_t old_value, word_t, word_t, word_t);
        static word_t getpid(word_t, word_t, word_t, word_t, word_t, word_t);
        static word_t sendfile(word_t out_fd, word_t in_fd, word_t offset, word_t count, word_t, word_t);
        static word_t socket(word_t domain, word_t type, word_t protocol, word_t, word_t, word_t);
        static word_t connect(word_t sockfd, word_t addr, word_t addrlen, word_t, word_t, word_t);
        static word_t accept(word_t sockfd, word_t addr, word_t addrlen, word_t, word_t, word_t);
        static word_t sendto(word_t sockfd, word_t buf, word_t len, word_t flags, word_t dest_addr, word_t addrlen, word_t, word_t);
        static word_t recvfrom(word_t sockfd, word_t buf, word_t len, word_t flags, word_t src_addr, word_t addrlen, word_t, word_t);
        static word_t sendmsg(word_t sockfd, word_t msg, word_t flags, word_t, word_t, word_t);
        static word_t recvmsg(word_t sockfd, word_t msg, word_t flags, word_t, word_t, word_t);
        static word_t shutdown(word_t sockfd, word_t how, word_t, word_t, word_t, word_t);
        static word_t bind(word_t sockfd, word_t addr, word_t addrlen, word_t, word_t, word_t);
        static word_t listen(word_t sockfd, word_t backlog, word_t, word_t, word_t, word_t);
        static word_t getsockname(word_t sockfd, word_t addr, word_t addrlen, word_t, word_t, word_t);
        static word_t getpeername(word_t sockfd, word_t addr, word_t addrlen, word_t, word_t, word_t);
        static word_t socketpair(word_t domain, word_t type, word_t protocol, word_t sv, word_t, word_t);
        static word_t setsockopt(word_t sockfd, word_t level, word_t optname, word_t optval, word_t optlen, word_t);
        static word_t getsockopt(word_t sockfd, word_t level, word_t optname, word_t optval, word_t optlen, word_t);
        static word_t clone(word_t flags, word_t newsp, word_t parent_tid, word_t child_tid, word_t exit_signal, word_t stack, word_t ptid, word_t newtls, word_t, word_t);
        static word_t fork(word_t, word_t, word_t, word_t, word_t, word_t);
        static word_t vfork(word_t, word_t, word_t, word_t, word_t, word_t);
        static word_t execve(word_t filename, word_t argv, word_t envp, word_t, word_t, word_t);
        static word_t exit(word_t status, word_t, word_t, word_t, word_t, word_t);
        static word_t wait4(word_t pid, word_t status, word_t options, word_t rusage, word_t, word_t);
        static word_t kill(word_t pid, word_t sig, word_t, word_t, word_t, word_t);
        static word_t uname(word_t buf, word_t, word_t, word_t, word_t, word_t);
        static word_t semget(word_t key, word_t nsems, word_t semflg, word_t, word_t, word_t);
        static word_t semop(word_t semid, word_t tsops, word_t nsops, word_t, word_t, word_t);
        static word_t semctl(word_t semid, word_t semnum, word_t cmd, word_t arg, word_t, word_t);
        static word_t shmdt(word_t shmaddr, word_t, word_t, word_t, word_t, word_t);
        static word_t msgget(word_t key, word_t msgflg, word_t, word_t, word_t, word_t);
        static word_t msgsnd(word_t msqid, word_t msgp, word_t msgsz, word_t msgflg, word_t, word_t);
        static word_t msgrcv(word_t msqid, word_t msgp, word_t msgsz, word_t msgtyp, word_t msgflg, word_t);
        static word_t msgctl(word_t msqid, word_t cmd, word_t buf, word_t, word_t, word_t);
        static word_t fcntl(word_t fd, word_t cmd, word_t arg, word_t, word_t, word_t);
        static word_t flock(word_t fd, word_t operation, word_t, word_t, word_t, word_t);
        static word_t fsync(word_t fd, word_t, word_t, word_t, word_t, word_t);
        static word_t fdatasync(word_t fd, word_t, word_t, word_t, word_t, word_t);
    }; */
} // namespace Hamster

