/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than this function does.
 */

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

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
int
runprogram(char *progname, char** args, unsigned long nargs)
{
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;

	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, &v);
	if (result) {
		return result;
	}

	/* We should be a new thread. */
	assert(curthread->t_vmspace == NULL);

	/* Create a new address space. */
	curthread->t_vmspace = as_create();
	if (curthread->t_vmspace==NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	/* Activate it. */
	as_activate(curthread->t_vmspace);

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	//TODO: this properly thread/sys exit
	//vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(curthread->t_vmspace, &stackptr);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		return result;
	}

	/*
	 * This is the beginning of our code to deal with arguments
	 */
    
    //Put the arguments themselves on the stack, and each time fill the args array with the address on the stack
	int i = 0;
	for(i = (nargs-1); i >= 0;) {
        //Adjust sp (stack ptr) by number of bytes needed for the stack with padding (for alignment)
        stackptr -= (strlen(args[i])  + 1 - (strlen(args[i]) + 1) % 4 + 4);
        
        //Copy current arg to the stack, fail if there is a problem copying, and keep track of mem location
		int fail = copyoutstr(args[i], stackptr, strlen(args[i]) + 1, NULL);
        if(fail) return fail;
        args[i--] = (char*)stackptr;
	}

    //Put NULL on the space between the args and the addresses (will terminate addresses)
    stackptr -= sizeof(void*); //Mem size of ptr should be 4 bytes (i.e. sp -= 4) but this is more robust
    *((int *)stackptr) = NULL;

    //Put the addresses on the stack (below the args)
	for(i = (nargs - 1); i >= 0; i--) {
        //Mem size of ptr should be 4 bytes (i.e. sp -= 4) but this is more robust
        stackptr -= sizeof(void*);
        
        //Copy current arg to the stack, fail if there is a problem copying, and keep track of mem location
		int fail = copyout(&args[i], stackptr, sizeof(vaddr_t));
		if(fail) return fail;
    }
    
    /*
	 * This is the end of our code to deal with arguments
     * If we come back from user mode we don't really need the last return
     * since panic will kill the OS anyway
	 */

	/* Warp to user mode. */
	md_usermode(nargs, (userptr_t)stackptr, stackptr, entrypoint);
	
	/* md_usermode does not return */
	panic("md_usermode returned\n");
	return EINVAL;
}

