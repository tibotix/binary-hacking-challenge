#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "interrupts.h"
#include "stl.h"

#define SYSCALL_IRQ 0x80

void syscall_isr(u64 error_code, saved_user_regs *regs);

void minik_init_syscalls(void);

struct iovec {
  void *iov_base; /* Starting address */
  u64 iov_len;    /* Size of the memory pointed to by iov_base. */
};

u64 sys$read(int fd, void *buf, u64 count);
u64 sys$write(int fd, void const *buf, u64 count);
static void sys$open() { __asm__("hlt"); }
static void sys$close() { __asm__("hlt"); }
static void sys$stat() { __asm__("hlt"); }
static void sys$fstat() { __asm__("hlt"); }
static void sys$lstat() { __asm__("hlt"); }
static void sys$poll() { __asm__("hlt"); }
static void sys$lseek() { __asm__("hlt"); }
static void sys$mmap() { __asm__("hlt"); }
static void sys$mprotect() { __asm__("hlt"); }
static void sys$munmap() { __asm__("hlt"); }
static void sys$brk() { __asm__("hlt"); }
static void sys$rt_sigaction() { __asm__("hlt"); }
static void sys$rt_sigprocmask() { __asm__("hlt"); }
static void sys$rt_sigreturn() { __asm__("hlt"); }
int sys$ioctl(int fd, int op, ...);
static void sys$pread64() { __asm__("hlt"); }
static void sys$pwrite64() { __asm__("hlt"); }
u64 sys$readv(int fd, const struct iovec *iov, int iovcnt);
u64 sys$writev(int fd, const struct iovec *iov, int iovcnt);
static void sys$access() { __asm__("hlt"); }
static void sys$pipe() { __asm__("hlt"); }
static void sys$select() { __asm__("hlt"); }
static void sys$sched_yield() { __asm__("hlt"); }
static void sys$mremap() { __asm__("hlt"); }
static void sys$msync() { __asm__("hlt"); }
static void sys$mincore() { __asm__("hlt"); }
static void sys$madvise() { __asm__("hlt"); }
static void sys$shmget() { __asm__("hlt"); }
static void sys$shmat() { __asm__("hlt"); }
static void sys$shmctl() { __asm__("hlt"); }
static void sys$dup() { __asm__("hlt"); }
static void sys$dup2() { __asm__("hlt"); }
static void sys$pause() { __asm__("hlt"); }
static void sys$nanosleep() { __asm__("hlt"); }
static void sys$getitimer() { __asm__("hlt"); }
static void sys$alarm() { __asm__("hlt"); }
static void sys$setitimer() { __asm__("hlt"); }
static void sys$getpid() { __asm__("hlt"); }
static void sys$sendfile() { __asm__("hlt"); }
static void sys$socket() { __asm__("hlt"); }
static void sys$connect() { __asm__("hlt"); }
static void sys$accept() { __asm__("hlt"); }
static void sys$sendto() { __asm__("hlt"); }
static void sys$recvfrom() { __asm__("hlt"); }
static void sys$sendmsg() { __asm__("hlt"); }
static void sys$recvmsg() { __asm__("hlt"); }
static void sys$shutdown() { __asm__("hlt"); }
static void sys$bind() { __asm__("hlt"); }
static void sys$listen() { __asm__("hlt"); }
static void sys$getsockname() { __asm__("hlt"); }
static void sys$getpeername() { __asm__("hlt"); }
static void sys$socketpair() { __asm__("hlt"); }
static void sys$setsockopt() { __asm__("hlt"); }
static void sys$getsockopt() { __asm__("hlt"); }
static void sys$clone() { __asm__("hlt"); }
static void sys$fork() { __asm__("hlt"); }
static void sys$vfork() { __asm__("hlt"); }
static void sys$execve() { __asm__("hlt"); }
static void sys$exit() { __asm__("hlt"); }
static void sys$wait4() { __asm__("hlt"); }
static void sys$kill() { __asm__("hlt"); }
static void sys$uname() { __asm__("hlt"); }
static void sys$semget() { __asm__("hlt"); }
static void sys$semop() { __asm__("hlt"); }
static void sys$semctl() { __asm__("hlt"); }
static void sys$shmdt() { __asm__("hlt"); }
static void sys$msgget() { __asm__("hlt"); }
static void sys$msgsnd() { __asm__("hlt"); }
static void sys$msgrcv() { __asm__("hlt"); }
static void sys$msgctl() { __asm__("hlt"); }
static void sys$fcntl() { __asm__("hlt"); }
static void sys$flock() { __asm__("hlt"); }
static void sys$fsync() { __asm__("hlt"); }
static void sys$fdatasync() { __asm__("hlt"); }
static void sys$truncate() { __asm__("hlt"); }
static void sys$ftruncate() { __asm__("hlt"); }
static void sys$getdents() { __asm__("hlt"); }
static void sys$getcwd() { __asm__("hlt"); }
static void sys$chdir() { __asm__("hlt"); }
static void sys$fchdir() { __asm__("hlt"); }
static void sys$rename() { __asm__("hlt"); }
static void sys$mkdir() { __asm__("hlt"); }
static void sys$rmdir() { __asm__("hlt"); }
static void sys$creat() { __asm__("hlt"); }
static void sys$link() { __asm__("hlt"); }
static void sys$unlink() { __asm__("hlt"); }
static void sys$symlink() { __asm__("hlt"); }
static void sys$readlink() { __asm__("hlt"); }
static void sys$chmod() { __asm__("hlt"); }
static void sys$fchmod() { __asm__("hlt"); }
static void sys$chown() { __asm__("hlt"); }
static void sys$fchown() { __asm__("hlt"); }
static void sys$lchown() { __asm__("hlt"); }
static void sys$umask() { __asm__("hlt"); }
static void sys$gettimeofday() { __asm__("hlt"); }
static void sys$getrlimit() { __asm__("hlt"); }
static void sys$getrusage() { __asm__("hlt"); }
static void sys$sysinfo() { __asm__("hlt"); }
static void sys$times() { __asm__("hlt"); }
static void sys$ptrace() { __asm__("hlt"); }
static void sys$getuid() { __asm__("hlt"); }
static void sys$syslog() { __asm__("hlt"); }
static void sys$getgid() { __asm__("hlt"); }
static void sys$setuid() { __asm__("hlt"); }
static void sys$setgid() { __asm__("hlt"); }
static void sys$geteuid() { __asm__("hlt"); }
static void sys$getegid() { __asm__("hlt"); }
static void sys$setpgid() { __asm__("hlt"); }
static void sys$getppid() { __asm__("hlt"); }
static void sys$getpgrp() { __asm__("hlt"); }
static void sys$setsid() { __asm__("hlt"); }
static void sys$setreuid() { __asm__("hlt"); }
static void sys$setregid() { __asm__("hlt"); }
static void sys$getgroups() { __asm__("hlt"); }
static void sys$setgroups() { __asm__("hlt"); }
static void sys$setresuid() { __asm__("hlt"); }
static void sys$getresuid() { __asm__("hlt"); }
static void sys$setresgid() { __asm__("hlt"); }
static void sys$getresgid() { __asm__("hlt"); }
static void sys$getpgid() { __asm__("hlt"); }
static void sys$setfsuid() { __asm__("hlt"); }
static void sys$setfsgid() { __asm__("hlt"); }
static void sys$getsid() { __asm__("hlt"); }
static void sys$capget() { __asm__("hlt"); }
static void sys$capset() { __asm__("hlt"); }
static void sys$rt_sigpending() { __asm__("hlt"); }
static void sys$rt_sigtimedwait() { __asm__("hlt"); }
static void sys$rt_sigqueueinfo() { __asm__("hlt"); }
static void sys$rt_sigsuspend() { __asm__("hlt"); }
static void sys$sigaltstack() { __asm__("hlt"); }
static void sys$utime() { __asm__("hlt"); }
static void sys$mknod() { __asm__("hlt"); }
static void sys$uselib() { __asm__("hlt"); }
static void sys$personality() { __asm__("hlt"); }
static void sys$ustat() { __asm__("hlt"); }
static void sys$statfs() { __asm__("hlt"); }
static void sys$fstatfs() { __asm__("hlt"); }
static void sys$sysfs() { __asm__("hlt"); }
static void sys$getpriority() { __asm__("hlt"); }
static void sys$setpriority() { __asm__("hlt"); }
static void sys$sched_setparam() { __asm__("hlt"); }
static void sys$sched_getparam() { __asm__("hlt"); }
static void sys$sched_setscheduler() { __asm__("hlt"); }
static void sys$sched_getscheduler() { __asm__("hlt"); }
static void sys$sched_get_priority_max() { __asm__("hlt"); }
static void sys$sched_get_priority_min() { __asm__("hlt"); }
static void sys$sched_rr_get_interval() { __asm__("hlt"); }
static void sys$mlock() { __asm__("hlt"); }
static void sys$munlock() { __asm__("hlt"); }
static void sys$mlockall() { __asm__("hlt"); }
static void sys$munlockall() { __asm__("hlt"); }
static void sys$vhangup() { __asm__("hlt"); }
static void sys$modify_ldt() { __asm__("hlt"); }
static void sys$pivot_root() { __asm__("hlt"); }
static void sys$_sysctl() { __asm__("hlt"); }
static void sys$prctl() { __asm__("hlt"); }
static void sys$arch_prctl() { __asm__("hlt"); }
static void sys$adjtimex() { __asm__("hlt"); }
static void sys$setrlimit() { __asm__("hlt"); }
static void sys$chroot() { __asm__("hlt"); }
static void sys$sync() { __asm__("hlt"); }
static void sys$acct() { __asm__("hlt"); }
static void sys$settimeofday() { __asm__("hlt"); }
static void sys$mount() { __asm__("hlt"); }
static void sys$umount2() { __asm__("hlt"); }
static void sys$swapon() { __asm__("hlt"); }
static void sys$swapoff() { __asm__("hlt"); }
static void sys$reboot() { __asm__("hlt"); }
static void sys$sethostname() { __asm__("hlt"); }
static void sys$setdomainname() { __asm__("hlt"); }
static void sys$iopl() { __asm__("hlt"); }
static void sys$ioperm() { __asm__("hlt"); }
static void sys$create_module() { __asm__("hlt"); }
static void sys$init_module() { __asm__("hlt"); }
static void sys$delete_module() { __asm__("hlt"); }
static void sys$get_kernel_syms() { __asm__("hlt"); }
static void sys$query_module() { __asm__("hlt"); }
static void sys$quotactl() { __asm__("hlt"); }
static void sys$nfsservctl() { __asm__("hlt"); }
static void sys$getpmsg() { __asm__("hlt"); }
static void sys$putpmsg() { __asm__("hlt"); }
static void sys$afs_syscall() { __asm__("hlt"); }
static void sys$tuxcall() { __asm__("hlt"); }
static void sys$security() { __asm__("hlt"); }
static void sys$gettid() { __asm__("hlt"); }
static void sys$readahead() { __asm__("hlt"); }
static void sys$setxattr() { __asm__("hlt"); }
static void sys$lsetxattr() { __asm__("hlt"); }
static void sys$fsetxattr() { __asm__("hlt"); }
static void sys$getxattr() { __asm__("hlt"); }
static void sys$lgetxattr() { __asm__("hlt"); }
static void sys$fgetxattr() { __asm__("hlt"); }
static void sys$listxattr() { __asm__("hlt"); }
static void sys$llistxattr() { __asm__("hlt"); }
static void sys$flistxattr() { __asm__("hlt"); }
static void sys$removexattr() { __asm__("hlt"); }
static void sys$lremovexattr() { __asm__("hlt"); }
static void sys$fremovexattr() { __asm__("hlt"); }
static void sys$tkill() { __asm__("hlt"); }
static void sys$time() { __asm__("hlt"); }
static void sys$futex() { __asm__("hlt"); }
static void sys$sched_setaffinity() { __asm__("hlt"); }
static void sys$sched_getaffinity() { __asm__("hlt"); }
static void sys$set_thread_area() { __asm__("hlt"); }
static void sys$io_setup() { __asm__("hlt"); }
static void sys$io_destroy() { __asm__("hlt"); }
static void sys$io_getevents() { __asm__("hlt"); }
static void sys$io_submit() { __asm__("hlt"); }
static void sys$io_cancel() { __asm__("hlt"); }
static void sys$get_thread_area() { __asm__("hlt"); }
static void sys$lookup_dcookie() { __asm__("hlt"); }
static void sys$epoll_create() { __asm__("hlt"); }
static void sys$epoll_ctl_old() { __asm__("hlt"); }
static void sys$epoll_wait_old() { __asm__("hlt"); }
static void sys$remap_file_pages() { __asm__("hlt"); }
static void sys$getdents64() { __asm__("hlt"); }
static void sys$set_tid_address() { __asm__("hlt"); }
static void sys$restart_syscall() { __asm__("hlt"); }
static void sys$semtimedop() { __asm__("hlt"); }
static void sys$fadvise64() { __asm__("hlt"); }
static void sys$timer_create() { __asm__("hlt"); }
static void sys$timer_settime() { __asm__("hlt"); }
static void sys$timer_gettime() { __asm__("hlt"); }
static void sys$timer_getoverrun() { __asm__("hlt"); }
static void sys$timer_delete() { __asm__("hlt"); }
static void sys$clock_settime() { __asm__("hlt"); }
static void sys$clock_gettime() { __asm__("hlt"); }
static void sys$clock_getres() { __asm__("hlt"); }
static void sys$clock_nanosleep() { __asm__("hlt"); }
static void sys$exit_group() { __asm__("hlt"); }
static void sys$epoll_wait() { __asm__("hlt"); }
static void sys$epoll_ctl() { __asm__("hlt"); }
static void sys$tgkill() { __asm__("hlt"); }
static void sys$utimes() { __asm__("hlt"); }
static void sys$vserver() { __asm__("hlt"); }
static void sys$mbind() { __asm__("hlt"); }
static void sys$set_mempolicy() { __asm__("hlt"); }
static void sys$get_mempolicy() { __asm__("hlt"); }
static void sys$mq_open() { __asm__("hlt"); }
static void sys$mq_unlink() { __asm__("hlt"); }
static void sys$mq_timedsend() { __asm__("hlt"); }
static void sys$mq_timedreceive() { __asm__("hlt"); }
static void sys$mq_notify() { __asm__("hlt"); }
static void sys$mq_getsetattr() { __asm__("hlt"); }
static void sys$kexec_load() { __asm__("hlt"); }
static void sys$waitid() { __asm__("hlt"); }
static void sys$add_key() { __asm__("hlt"); }
static void sys$request_key() { __asm__("hlt"); }
static void sys$keyctl() { __asm__("hlt"); }
static void sys$ioprio_set() { __asm__("hlt"); }
static void sys$ioprio_get() { __asm__("hlt"); }
static void sys$inotify_init() { __asm__("hlt"); }
static void sys$inotify_add_watch() { __asm__("hlt"); }
static void sys$inotify_rm_watch() { __asm__("hlt"); }
static void sys$migrate_pages() { __asm__("hlt"); }
static void sys$openat() { __asm__("hlt"); }
static void sys$mkdirat() { __asm__("hlt"); }
static void sys$mknodat() { __asm__("hlt"); }
static void sys$fchownat() { __asm__("hlt"); }
static void sys$futimesat() { __asm__("hlt"); }
static void sys$newfstatat() { __asm__("hlt"); }
static void sys$unlinkat() { __asm__("hlt"); }
static void sys$renameat() { __asm__("hlt"); }
static void sys$linkat() { __asm__("hlt"); }
static void sys$symlinkat() { __asm__("hlt"); }
static void sys$readlinkat() { __asm__("hlt"); }
static void sys$fchmodat() { __asm__("hlt"); }
static void sys$faccessat() { __asm__("hlt"); }
static void sys$pselect6() { __asm__("hlt"); }
static void sys$ppoll() { __asm__("hlt"); }
static void sys$unshare() { __asm__("hlt"); }
static void sys$set_robust_list() { __asm__("hlt"); }
static void sys$get_robust_list() { __asm__("hlt"); }
static void sys$splice() { __asm__("hlt"); }
static void sys$tee() { __asm__("hlt"); }
static void sys$sync_file_range() { __asm__("hlt"); }
static void sys$vmsplice() { __asm__("hlt"); }
static void sys$move_pages() { __asm__("hlt"); }
static void sys$utimensat() { __asm__("hlt"); }
static void sys$epoll_pwait() { __asm__("hlt"); }
static void sys$signalfd() { __asm__("hlt"); }
static void sys$timerfd_create() { __asm__("hlt"); }
static void sys$eventfd() { __asm__("hlt"); }
static void sys$fallocate() { __asm__("hlt"); }
static void sys$timerfd_settime() { __asm__("hlt"); }
static void sys$timerfd_gettime() { __asm__("hlt"); }
static void sys$accept4() { __asm__("hlt"); }
static void sys$signalfd4() { __asm__("hlt"); }
static void sys$eventfd2() { __asm__("hlt"); }
static void sys$epoll_create1() { __asm__("hlt"); }
static void sys$dup3() { __asm__("hlt"); }
static void sys$pipe2() { __asm__("hlt"); }
static void sys$inotify_init1() { __asm__("hlt"); }
static void sys$preadv() { __asm__("hlt"); }
static void sys$pwritev() { __asm__("hlt"); }
static void sys$rt_tgsigqueueinfo() { __asm__("hlt"); }
static void sys$perf_event_open() { __asm__("hlt"); }
static void sys$recvmmsg() { __asm__("hlt"); }
static void sys$fanotify_init() { __asm__("hlt"); }
static void sys$fanotify_mark() { __asm__("hlt"); }
static void sys$prlimit64() { __asm__("hlt"); }
static void sys$name_to_handle_at() { __asm__("hlt"); }
static void sys$open_by_handle_at() { __asm__("hlt"); }
static void sys$clock_adjtime() { __asm__("hlt"); }
static void sys$syncfs() { __asm__("hlt"); }
static void sys$sendmmsg() { __asm__("hlt"); }
static void sys$setns() { __asm__("hlt"); }
static void sys$getcpu() { __asm__("hlt"); }
static void sys$process_vm_readv() { __asm__("hlt"); }
static void sys$process_vm_writev() { __asm__("hlt"); }
static void sys$kcmp() { __asm__("hlt"); }
static void sys$finit_module() { __asm__("hlt"); }
static void sys$sched_setattr() { __asm__("hlt"); }
static void sys$sched_getattr() { __asm__("hlt"); }
static void sys$renameat2() { __asm__("hlt"); }
static void sys$seccomp() { __asm__("hlt"); }
static void sys$getrandom() { __asm__("hlt"); }
static void sys$memfd_create() { __asm__("hlt"); }
static void sys$kexec_file_load() { __asm__("hlt"); }
static void sys$bpf() { __asm__("hlt"); }
static void sys$execveat() { __asm__("hlt"); }
static void sys$userfaultfd() { __asm__("hlt"); }
static void sys$membarrier() { __asm__("hlt"); }
static void sys$mlock2() { __asm__("hlt"); }
static void sys$copy_file_range() { __asm__("hlt"); }
static void sys$preadv2() { __asm__("hlt"); }
static void sys$pwritev2() { __asm__("hlt"); }
static void sys$pkey_mprotect() { __asm__("hlt"); }
static void sys$pkey_alloc() { __asm__("hlt"); }
static void sys$pkey_free() { __asm__("hlt"); }
static void sys$statx() { __asm__("hlt"); }
static void sys$io_pgetevents() { __asm__("hlt"); }
static void sys$rseq() { __asm__("hlt"); }
static void sys$pidfd_send_signal() { __asm__("hlt"); }
static void sys$io_uring_setup() { __asm__("hlt"); }
static void sys$io_uring_enter() { __asm__("hlt"); }
static void sys$io_uring_register() { __asm__("hlt"); }
static void sys$open_tree() { __asm__("hlt"); }
static void sys$move_mount() { __asm__("hlt"); }
static void sys$fsopen() { __asm__("hlt"); }
static void sys$fsconfig() { __asm__("hlt"); }
static void sys$fsmount() { __asm__("hlt"); }
static void sys$fspick() { __asm__("hlt"); }
static void sys$pidfd_open() { __asm__("hlt"); }
static void sys$clone3() { __asm__("hlt"); }
static void sys$close_range() { __asm__("hlt"); }
static void sys$openat2() { __asm__("hlt"); }
static void sys$pidfd_getfd() { __asm__("hlt"); }
static void sys$faccessat2() { __asm__("hlt"); }
static void sys$process_madvise() { __asm__("hlt"); }
static void sys$epoll_pwait2() { __asm__("hlt"); }
static void sys$mount_setattr() { __asm__("hlt"); }
static void sys$quotactl_fd() { __asm__("hlt"); }
static void sys$landlock_create_ruleset() { __asm__("hlt"); }
static void sys$landlock_add_rule() { __asm__("hlt"); }
static void sys$landlock_restrict_self() { __asm__("hlt"); }
static void sys$memfd_secret() { __asm__("hlt"); }
static void sys$process_mrelease() { __asm__("hlt"); }
static void sys$futex_waitv() { __asm__("hlt"); }
static void sys$set_mempolicy_home_node() { __asm__("hlt"); }
static void sys$cachestat() { __asm__("hlt"); }
static void sys$fchmodat2() { __asm__("hlt"); }
static void sys$map_shadow_stack() { __asm__("hlt"); }
static void sys$futex_wake() { __asm__("hlt"); }
static void sys$futex_wait() { __asm__("hlt"); }
static void sys$futex_requeue() { __asm__("hlt"); }

typedef u64 (*syscall_handler)();
static syscall_handler syscall_table[] = {
    (syscall_handler)&sys$read,
    (syscall_handler)&sys$write,
    (syscall_handler)&sys$open,
    (syscall_handler)&sys$close,
    (syscall_handler)&sys$stat,
    (syscall_handler)&sys$fstat,
    (syscall_handler)&sys$lstat,
    (syscall_handler)&sys$poll,
    (syscall_handler)&sys$lseek,
    (syscall_handler)&sys$mmap,
    (syscall_handler)&sys$mprotect,
    (syscall_handler)&sys$munmap,
    (syscall_handler)&sys$brk,
    (syscall_handler)&sys$rt_sigaction,
    (syscall_handler)&sys$rt_sigprocmask,
    (syscall_handler)&sys$rt_sigreturn,
    (syscall_handler)&sys$ioctl,
    (syscall_handler)&sys$pread64,
    (syscall_handler)&sys$pwrite64,
    (syscall_handler)&sys$readv,
    (syscall_handler)&sys$writev,
    (syscall_handler)&sys$access,
    (syscall_handler)&sys$pipe,
    (syscall_handler)&sys$select,
    (syscall_handler)&sys$sched_yield,
    (syscall_handler)&sys$mremap,
    (syscall_handler)&sys$msync,
    (syscall_handler)&sys$mincore,
    (syscall_handler)&sys$madvise,
    (syscall_handler)&sys$shmget,
    (syscall_handler)&sys$shmat,
    (syscall_handler)&sys$shmctl,
    (syscall_handler)&sys$dup,
    (syscall_handler)&sys$dup2,
    (syscall_handler)&sys$pause,
    (syscall_handler)&sys$nanosleep,
    (syscall_handler)&sys$getitimer,
    (syscall_handler)&sys$alarm,
    (syscall_handler)&sys$setitimer,
    (syscall_handler)&sys$getpid,
    (syscall_handler)&sys$sendfile,
    (syscall_handler)&sys$socket,
    (syscall_handler)&sys$connect,
    (syscall_handler)&sys$accept,
    (syscall_handler)&sys$sendto,
    (syscall_handler)&sys$recvfrom,
    (syscall_handler)&sys$sendmsg,
    (syscall_handler)&sys$recvmsg,
    (syscall_handler)&sys$shutdown,
    (syscall_handler)&sys$bind,
    (syscall_handler)&sys$listen,
    (syscall_handler)&sys$getsockname,
    (syscall_handler)&sys$getpeername,
    (syscall_handler)&sys$socketpair,
    (syscall_handler)&sys$setsockopt,
    (syscall_handler)&sys$getsockopt,
    (syscall_handler)&sys$clone,
    (syscall_handler)&sys$fork,
    (syscall_handler)&sys$vfork,
    (syscall_handler)&sys$execve,
    (syscall_handler)&sys$exit,
    (syscall_handler)&sys$wait4,
    (syscall_handler)&sys$kill,
    (syscall_handler)&sys$uname,
    (syscall_handler)&sys$semget,
    (syscall_handler)&sys$semop,
    (syscall_handler)&sys$semctl,
    (syscall_handler)&sys$shmdt,
    (syscall_handler)&sys$msgget,
    (syscall_handler)&sys$msgsnd,
    (syscall_handler)&sys$msgrcv,
    (syscall_handler)&sys$msgctl,
    (syscall_handler)&sys$fcntl,
    (syscall_handler)&sys$flock,
    (syscall_handler)&sys$fsync,
    (syscall_handler)&sys$fdatasync,
    (syscall_handler)&sys$truncate,
    (syscall_handler)&sys$ftruncate,
    (syscall_handler)&sys$getdents,
    (syscall_handler)&sys$getcwd,
    (syscall_handler)&sys$chdir,
    (syscall_handler)&sys$fchdir,
    (syscall_handler)&sys$rename,
    (syscall_handler)&sys$mkdir,
    (syscall_handler)&sys$rmdir,
    (syscall_handler)&sys$creat,
    (syscall_handler)&sys$link,
    (syscall_handler)&sys$unlink,
    (syscall_handler)&sys$symlink,
    (syscall_handler)&sys$readlink,
    (syscall_handler)&sys$chmod,
    (syscall_handler)&sys$fchmod,
    (syscall_handler)&sys$chown,
    (syscall_handler)&sys$fchown,
    (syscall_handler)&sys$lchown,
    (syscall_handler)&sys$umask,
    (syscall_handler)&sys$gettimeofday,
    (syscall_handler)&sys$getrlimit,
    (syscall_handler)&sys$getrusage,
    (syscall_handler)&sys$sysinfo,
    (syscall_handler)&sys$times,
    (syscall_handler)&sys$ptrace,
    (syscall_handler)&sys$getuid,
    (syscall_handler)&sys$syslog,
    (syscall_handler)&sys$getgid,
    (syscall_handler)&sys$setuid,
    (syscall_handler)&sys$setgid,
    (syscall_handler)&sys$geteuid,
    (syscall_handler)&sys$getegid,
    (syscall_handler)&sys$setpgid,
    (syscall_handler)&sys$getppid,
    (syscall_handler)&sys$getpgrp,
    (syscall_handler)&sys$setsid,
    (syscall_handler)&sys$setreuid,
    (syscall_handler)&sys$setregid,
    (syscall_handler)&sys$getgroups,
    (syscall_handler)&sys$setgroups,
    (syscall_handler)&sys$setresuid,
    (syscall_handler)&sys$getresuid,
    (syscall_handler)&sys$setresgid,
    (syscall_handler)&sys$getresgid,
    (syscall_handler)&sys$getpgid,
    (syscall_handler)&sys$setfsuid,
    (syscall_handler)&sys$setfsgid,
    (syscall_handler)&sys$getsid,
    (syscall_handler)&sys$capget,
    (syscall_handler)&sys$capset,
    (syscall_handler)&sys$rt_sigpending,
    (syscall_handler)&sys$rt_sigtimedwait,
    (syscall_handler)&sys$rt_sigqueueinfo,
    (syscall_handler)&sys$rt_sigsuspend,
    (syscall_handler)&sys$sigaltstack,
    (syscall_handler)&sys$utime,
    (syscall_handler)&sys$mknod,
    (syscall_handler)&sys$uselib,
    (syscall_handler)&sys$personality,
    (syscall_handler)&sys$ustat,
    (syscall_handler)&sys$statfs,
    (syscall_handler)&sys$fstatfs,
    (syscall_handler)&sys$sysfs,
    (syscall_handler)&sys$getpriority,
    (syscall_handler)&sys$setpriority,
    (syscall_handler)&sys$sched_setparam,
    (syscall_handler)&sys$sched_getparam,
    (syscall_handler)&sys$sched_setscheduler,
    (syscall_handler)&sys$sched_getscheduler,
    (syscall_handler)&sys$sched_get_priority_max,
    (syscall_handler)&sys$sched_get_priority_min,
    (syscall_handler)&sys$sched_rr_get_interval,
    (syscall_handler)&sys$mlock,
    (syscall_handler)&sys$munlock,
    (syscall_handler)&sys$mlockall,
    (syscall_handler)&sys$munlockall,
    (syscall_handler)&sys$vhangup,
    (syscall_handler)&sys$modify_ldt,
    (syscall_handler)&sys$pivot_root,
    (syscall_handler)&sys$_sysctl,
    (syscall_handler)&sys$prctl,
    (syscall_handler)&sys$arch_prctl,
    (syscall_handler)&sys$adjtimex,
    (syscall_handler)&sys$setrlimit,
    (syscall_handler)&sys$chroot,
    (syscall_handler)&sys$sync,
    (syscall_handler)&sys$acct,
    (syscall_handler)&sys$settimeofday,
    (syscall_handler)&sys$mount,
    (syscall_handler)&sys$umount2,
    (syscall_handler)&sys$swapon,
    (syscall_handler)&sys$swapoff,
    (syscall_handler)&sys$reboot,
    (syscall_handler)&sys$sethostname,
    (syscall_handler)&sys$setdomainname,
    (syscall_handler)&sys$iopl,
    (syscall_handler)&sys$ioperm,
    (syscall_handler)&sys$create_module,
    (syscall_handler)&sys$init_module,
    (syscall_handler)&sys$delete_module,
    (syscall_handler)&sys$get_kernel_syms,
    (syscall_handler)&sys$query_module,
    (syscall_handler)&sys$quotactl,
    (syscall_handler)&sys$nfsservctl,
    (syscall_handler)&sys$getpmsg,
    (syscall_handler)&sys$putpmsg,
    (syscall_handler)&sys$afs_syscall,
    (syscall_handler)&sys$tuxcall,
    (syscall_handler)&sys$security,
    (syscall_handler)&sys$gettid,
    (syscall_handler)&sys$readahead,
    (syscall_handler)&sys$setxattr,
    (syscall_handler)&sys$lsetxattr,
    (syscall_handler)&sys$fsetxattr,
    (syscall_handler)&sys$getxattr,
    (syscall_handler)&sys$lgetxattr,
    (syscall_handler)&sys$fgetxattr,
    (syscall_handler)&sys$listxattr,
    (syscall_handler)&sys$llistxattr,
    (syscall_handler)&sys$flistxattr,
    (syscall_handler)&sys$removexattr,
    (syscall_handler)&sys$lremovexattr,
    (syscall_handler)&sys$fremovexattr,
    (syscall_handler)&sys$tkill,
    (syscall_handler)&sys$time,
    (syscall_handler)&sys$futex,
    (syscall_handler)&sys$sched_setaffinity,
    (syscall_handler)&sys$sched_getaffinity,
    (syscall_handler)&sys$set_thread_area,
    (syscall_handler)&sys$io_setup,
    (syscall_handler)&sys$io_destroy,
    (syscall_handler)&sys$io_getevents,
    (syscall_handler)&sys$io_submit,
    (syscall_handler)&sys$io_cancel,
    (syscall_handler)&sys$get_thread_area,
    (syscall_handler)&sys$lookup_dcookie,
    (syscall_handler)&sys$epoll_create,
    (syscall_handler)&sys$epoll_ctl_old,
    (syscall_handler)&sys$epoll_wait_old,
    (syscall_handler)&sys$remap_file_pages,
    (syscall_handler)&sys$getdents64,
    (syscall_handler)&sys$set_tid_address,
    (syscall_handler)&sys$restart_syscall,
    (syscall_handler)&sys$semtimedop,
    (syscall_handler)&sys$fadvise64,
    (syscall_handler)&sys$timer_create,
    (syscall_handler)&sys$timer_settime,
    (syscall_handler)&sys$timer_gettime,
    (syscall_handler)&sys$timer_getoverrun,
    (syscall_handler)&sys$timer_delete,
    (syscall_handler)&sys$clock_settime,
    (syscall_handler)&sys$clock_gettime,
    (syscall_handler)&sys$clock_getres,
    (syscall_handler)&sys$clock_nanosleep,
    (syscall_handler)&sys$exit_group,
    (syscall_handler)&sys$epoll_wait,
    (syscall_handler)&sys$epoll_ctl,
    (syscall_handler)&sys$tgkill,
    (syscall_handler)&sys$utimes,
    (syscall_handler)&sys$vserver,
    (syscall_handler)&sys$mbind,
    (syscall_handler)&sys$set_mempolicy,
    (syscall_handler)&sys$get_mempolicy,
    (syscall_handler)&sys$mq_open,
    (syscall_handler)&sys$mq_unlink,
    (syscall_handler)&sys$mq_timedsend,
    (syscall_handler)&sys$mq_timedreceive,
    (syscall_handler)&sys$mq_notify,
    (syscall_handler)&sys$mq_getsetattr,
    (syscall_handler)&sys$kexec_load,
    (syscall_handler)&sys$waitid,
    (syscall_handler)&sys$add_key,
    (syscall_handler)&sys$request_key,
    (syscall_handler)&sys$keyctl,
    (syscall_handler)&sys$ioprio_set,
    (syscall_handler)&sys$ioprio_get,
    (syscall_handler)&sys$inotify_init,
    (syscall_handler)&sys$inotify_add_watch,
    (syscall_handler)&sys$inotify_rm_watch,
    (syscall_handler)&sys$migrate_pages,
    (syscall_handler)&sys$openat,
    (syscall_handler)&sys$mkdirat,
    (syscall_handler)&sys$mknodat,
    (syscall_handler)&sys$fchownat,
    (syscall_handler)&sys$futimesat,
    (syscall_handler)&sys$newfstatat,
    (syscall_handler)&sys$unlinkat,
    (syscall_handler)&sys$renameat,
    (syscall_handler)&sys$linkat,
    (syscall_handler)&sys$symlinkat,
    (syscall_handler)&sys$readlinkat,
    (syscall_handler)&sys$fchmodat,
    (syscall_handler)&sys$faccessat,
    (syscall_handler)&sys$pselect6,
    (syscall_handler)&sys$ppoll,
    (syscall_handler)&sys$unshare,
    (syscall_handler)&sys$set_robust_list,
    (syscall_handler)&sys$get_robust_list,
    (syscall_handler)&sys$splice,
    (syscall_handler)&sys$tee,
    (syscall_handler)&sys$sync_file_range,
    (syscall_handler)&sys$vmsplice,
    (syscall_handler)&sys$move_pages,
    (syscall_handler)&sys$utimensat,
    (syscall_handler)&sys$epoll_pwait,
    (syscall_handler)&sys$signalfd,
    (syscall_handler)&sys$timerfd_create,
    (syscall_handler)&sys$eventfd,
    (syscall_handler)&sys$fallocate,
    (syscall_handler)&sys$timerfd_settime,
    (syscall_handler)&sys$timerfd_gettime,
    (syscall_handler)&sys$accept4,
    (syscall_handler)&sys$signalfd4,
    (syscall_handler)&sys$eventfd2,
    (syscall_handler)&sys$epoll_create1,
    (syscall_handler)&sys$dup3,
    (syscall_handler)&sys$pipe2,
    (syscall_handler)&sys$inotify_init1,
    (syscall_handler)&sys$preadv,
    (syscall_handler)&sys$pwritev,
    (syscall_handler)&sys$rt_tgsigqueueinfo,
    (syscall_handler)&sys$perf_event_open,
    (syscall_handler)&sys$recvmmsg,
    (syscall_handler)&sys$fanotify_init,
    (syscall_handler)&sys$fanotify_mark,
    (syscall_handler)&sys$prlimit64,
    (syscall_handler)&sys$name_to_handle_at,
    (syscall_handler)&sys$open_by_handle_at,
    (syscall_handler)&sys$clock_adjtime,
    (syscall_handler)&sys$syncfs,
    (syscall_handler)&sys$sendmmsg,
    (syscall_handler)&sys$setns,
    (syscall_handler)&sys$getcpu,
    (syscall_handler)&sys$process_vm_readv,
    (syscall_handler)&sys$process_vm_writev,
    (syscall_handler)&sys$kcmp,
    (syscall_handler)&sys$finit_module,
    (syscall_handler)&sys$sched_setattr,
    (syscall_handler)&sys$sched_getattr,
    (syscall_handler)&sys$renameat2,
    (syscall_handler)&sys$seccomp,
    (syscall_handler)&sys$getrandom,
    (syscall_handler)&sys$memfd_create,
    (syscall_handler)&sys$kexec_file_load,
    (syscall_handler)&sys$bpf,
    (syscall_handler)&sys$execveat,
    (syscall_handler)&sys$userfaultfd,
    (syscall_handler)&sys$membarrier,
    (syscall_handler)&sys$mlock2,
    (syscall_handler)&sys$copy_file_range,
    (syscall_handler)&sys$preadv2,
    (syscall_handler)&sys$pwritev2,
    (syscall_handler)&sys$pkey_mprotect,
    (syscall_handler)&sys$pkey_alloc,
    (syscall_handler)&sys$pkey_free,
    (syscall_handler)&sys$statx,
    (syscall_handler)&sys$io_pgetevents,
    (syscall_handler)&sys$rseq,
    (syscall_handler)&sys$pidfd_send_signal,
    (syscall_handler)&sys$io_uring_setup,
    (syscall_handler)&sys$io_uring_enter,
    (syscall_handler)&sys$io_uring_register,
    (syscall_handler)&sys$open_tree,
    (syscall_handler)&sys$move_mount,
    (syscall_handler)&sys$fsopen,
    (syscall_handler)&sys$fsconfig,
    (syscall_handler)&sys$fsmount,
    (syscall_handler)&sys$fspick,
    (syscall_handler)&sys$pidfd_open,
    (syscall_handler)&sys$clone3,
    (syscall_handler)&sys$close_range,
    (syscall_handler)&sys$openat2,
    (syscall_handler)&sys$pidfd_getfd,
    (syscall_handler)&sys$faccessat2,
    (syscall_handler)&sys$process_madvise,
    (syscall_handler)&sys$epoll_pwait2,
    (syscall_handler)&sys$mount_setattr,
    (syscall_handler)&sys$quotactl_fd,
    (syscall_handler)&sys$landlock_create_ruleset,
    (syscall_handler)&sys$landlock_add_rule,
    (syscall_handler)&sys$landlock_restrict_self,
    (syscall_handler)&sys$memfd_secret,
    (syscall_handler)&sys$process_mrelease,
    (syscall_handler)&sys$futex_waitv,
    (syscall_handler)&sys$set_mempolicy_home_node,
    (syscall_handler)&sys$cachestat,
    (syscall_handler)&sys$fchmodat2,
    (syscall_handler)&sys$map_shadow_stack,
    (syscall_handler)&sys$futex_wake,
    (syscall_handler)&sys$futex_wait,
    (syscall_handler)&sys$futex_requeue,
};

#endif // SYSCALLS_H
