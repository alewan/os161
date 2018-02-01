#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <machine/pcb.h>
#include <machine/spl.h>
#include <machine/trapframe.h>
#include <kern/callno.h>
#include <syscall.h>
#include <oursyscall.h>


/*
 * System call handler.
 *
 * A pointer to the trapframe created during exception entry (in
 * exception.S) is passed in.
 *
 * The calling conventions for syscalls are as follows: Like ordinary
 * function calls, the first 4 32-bit arguments are passed in the 4
 * argument registers a0-a3. In addition, the system call number is
 * passed in the v0 register.
 *
 * On successful return, the return value is passed back in the v0
 * register, like an ordinary function call, and the a3 register is
 * also set to 0 to indicate success.
 *
 * On an error return, the error code is passed back in the v0
 * register, and the a3 register is set to 1 to indicate failure.
 * (Userlevel code takes care of storing the error code in errno and
 * returning the value -1 from the actual userlevel syscall function.
 * See src/lib/libc/syscalls.S and related files.)
 *
 * Upon syscall return the program counter stored in the trapframe
 * must be incremented by one instruction; otherwise the exception
 * return code will restart the "syscall" instruction and the system
 * call will repeat forever.
 *
 * Since none of the OS/161 system calls have more than 4 arguments,
 * there should be no need to fetch additional arguments from the
 * user-level stack.
 *
 * Watch out: if you make system calls that have 64-bit quantities as
 * arguments, they will get passed in pairs of registers, and not
 * necessarily in the way you expect. We recommend you don't do it.
 * (In fact, we recommend you don't use 64-bit quantities at all. See
 * arch/mips/include/types.h.)
 */

void
mips_syscall(struct trapframe *tf)
{
	int callno;
	int32_t retval;
	int err;


	assert(curspl==0);

	callno = tf->tf_v0;

	/*
	 * Initialize retval to 0. Many of the system calls don't
	 * really return a value, just 0 for success and -1 on
	 * error. Since retval is the value returned on success,
	 * initialize it to 0 by default; thus it's not necessary to
	 * deal with it except for calls that return other values, 
	 * like write.
	 */

	retval = 0;
	err = 0;

	switch (callno) {
	    case SYS_reboot:
		err = sys_reboot(tf->tf_a0);
		break;

		/* Add stuff here */
		//Adding sys calls and handlers

		//READ SYSCALL
		case SYS_read:
		//Read only allowed from STDIN else return BADF error
		if ((tf->tf_a0) != STDIN_FILENO) {
			err = EBADF;
			break;
		}
		//Call handler for read and set err, retval appropriately
		err = sys_read(tf->tf_a1, tf->tf_a2);
		retval = (err ? -1 : (tf->tf_a2));
		break;

		//WRITE SYSCALL
		case SYS_write:
		//Write only allowed from STDOUT or STDERR else return BADF error
		if (tf->tf_a0 != STDOUT_FILENO && tf->tf_a0 != STDERR_FILENO) {
			err = EBADF;
			break;
		}
		//Call handler for write and set err, retval appropriately
		err = sys_write(tf->tf_a1, tf->tf_a2);
		retval = (err ? -1 : (tf->tf_a2));		
		break;

		//FORK SYSCALL
		case SYS_fork:
		err = sys_fork(tf, &retval);
		break;

		case SYS__exit:
		err = sys__exit(tf->tf_a0);
		break;

		case SYS_execv:
		err = sys_execv(tf->tf_a0, tf->tf_a1, &retval);
		break;

		case SYS_getpid:
		retval = sys_getpid();
		err = 0;
		break;

		case SYS_waitpid:
		err = sys_waitpid(tf->tf_a0, tf->tf_a1, tf->tf_a2, &retval);
		break;

		case SYS___time:
		err = sys__time(tf->tf_a0,tf->tf_a1, &retval);
		break;

		case SYS_sbrk:
		err = sys_sbrk(tf->tf_a0, &retval);
		break;
 
	    default:
		kprintf("Unknown syscall %d\n", callno);
		err = ENOSYS;
		break;
	}


	if (err) {
		/*
		 * Return the error code. This gets converted at
		 * userlevel to a return value of -1 and the error
		 * code in errno.
		 */
		tf->tf_v0 = err;
		tf->tf_a3 = 1;      /* signal an error */
	}
	else {
		/* Success. */
		tf->tf_v0 = retval;
		tf->tf_a3 = 0;      /* signal no error */
	}
	
	/*
	 * Now, advance the program counter, to avoid restarting
	 * the syscall over and over again.
	 */
	
	tf->tf_epc += 4;

	/* Make sure the syscall code didn't forget to lower spl */
	assert(curspl==0);
}

void md_forkentry(trapframeptr tf, unsigned long addrspace) {
	/*
	 * This function is provided as a reminder. You need to write
	 * both it and the code that calls it.
	 *
	 * Thus, you can trash it and do things another way if you prefer.
	 */

	
	//Must do equivalent to what happens above in mips_syscall
	//Set retval and signal no error (see else statement from mips_syscall)
	tf->tf_v0 = 0;
	tf->tf_a3 = 0;
	// Move to next instruction (end of mips_syscall)
	tf->tf_epc += 4;

	//We must point to our own (child's) addrspace and activate it
	curthread->t_vmspace = (addrspaceptr) addrspace;
	as_activate(curthread->t_vmspace);
	
	// Create a new trapframe in a user-accessible space and set it to child's TF (shalow copy okay since no ptrs in TF)
	Trapframe newTF = *(tf);
	kfree(tf); //avoid mem leak

	//Go to user mode (should not return)
	mips_usermode(&newTF);

	return;
}
