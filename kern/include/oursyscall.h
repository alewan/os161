#ifndef OURSYSCALL_H
#define OURSYSCALL_H

#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <thread.h>
#include <curthread.h>
#include <vm.h>
#include <vfs.h>
#include <test.h>
#include <syscall.h>
#include <machine/trapframe.h>
#include <clock.h>

//Make typedefs available for TF and AS since they will be used a lot
typedef struct trapframe Trapframe;
typedef struct trapframe* trapframeptr;
typedef struct addrspace* addrspaceptr;

int sys_fork(trapframeptr, int*);
int sys_execv(const char*, char**, int*);
int sys_getpid(void);
int sys_waitpid(pid_t, int*, int, int*);
int sys__exit(int);
int sys_write(void*, size_t);
int sys_read(void*, size_t);
int sys__time(time_t*, unsigned long*, int*);
int sys_sbrk(intptr_t, int*);

#endif //OURSYSCALL_H
