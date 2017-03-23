#define _GNU_SOURCE

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>

#define ASM 0

#define PRINT_CNT 10
#define SYS_waitpid 0x07
#define STACK_SIZE (1024 * 1024) /* Stack size for cloned child */

#define err_exit(msg)       \
    do {                    \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)

ssize_t sys_write(int fd, const char *buf, size_t len) {
    return syscall(SYS_write, fd, buf, len);
}

int sys_exit(int error_code) { return syscall(SYS_exit, error_code); }

int sys_waitpid(int pid, int *stat_loc, int options) {
    return syscall(SYS_waitpid, pid, stat_loc, options);
}

int sys_exit_group(int error_code) {
    return syscall(SYS_exit_group, error_code);
}

/* Assembly equivalents */

ssize_t asm_exit(int error_code) {
    ssize_t ret;
    asm volatile("syscall"
                 : "=a"(ret)
                 : "0"(__NR_exit), "d"(error_code)
                 : "cc", "rbx");
    return ret;
}

ssize_t asm_waitpid(int pid, int *stat_loc, int options) {
    ssize_t ret;
    register int pid_ asm("rdi") = pid;
    asm volatile("syscall"
                 : "=a"(ret)
                 : "0"(SYS_waitpid), "g"(pid_), "g"(stat_loc), "g"(options)
                 : "cc", "rbx", "rcx", "rdx");
    return ret;
}

ssize_t asm_exit_group(int error_code) {
    ssize_t ret;
    asm volatile("syscall"
                 : "=a"(ret)
                 : "0"(__NR_exit_group), "d"(error_code)
                 : "cc", "rbx");
    return ret;
}

ssize_t asm_write(int fd, const void *buf, size_t size) {
    ssize_t ret;
    asm volatile("syscall"
                 : "=a"(ret)
                 : "0"(__NR_write), "D"(fd), "S"(buf), "d"(size)
                 : "cc", "rcx", "r11", "memory");
    return ret;
}

#if ASM == 1

#define sys_write asm_write
#define sys_exit asm_exit
#define sys_exit_group asm_exit_group
#define sys_waitpid asm_waitpid

#endif

static int child_func()
{
    int i;

    for (i = 0; i < PRINT_CNT; ++i) sys_write(1, "A\n", 2);

    sys_exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    int i;
    char *stack;     /* Start of stack buffer */
    char *stack_top; /* End of stack buffer */
    pid_t pid;
    struct utsname uts;

    /* Allocate stack for child */

    stack = malloc(STACK_SIZE);
    if (stack == NULL) err_exit("malloc");

    stack_top = stack + STACK_SIZE; /* Stack grows downward */

    pid = clone(child_func, stack_top, CLONE_VM | SIGCHLD, NULL);
    if (pid == -1) err_exit("clone");

    /* Parent falls through to here */

    for (i = 0; i < PRINT_CNT; ++i) sys_write(SYS_write, "B\n", 2);

    /* Wait for child */
    if (sys_waitpid(pid, NULL, 0) == -1) err_exit("waitpid");

    sys_exit_group(EXIT_SUCCESS);
}
