#include <oursyscall.h>
#include <ourextern.h>
#include <kern/limits.h>
#include <addrspace.h>

//Handle write using copyin to copy from buffer to temp (checks validity of user buf ptr) and print if successful (else return with error)
int sys_write(void* buf, size_t nbytes) {
    char* kernbuf = kmalloc(sizeof(char)*nbytes);
    if (kernbuf == NULL) return ENOMEM;
    
    int failure = copyin(buf, kernbuf, nbytes);
    if (failure) {
        kfree(kernbuf);
        return failure;
    }
    else {
        size_t i;
        for(i = 0; i < nbytes; i++) {
            kprintf("%c", *(kernbuf + i));
        }
        kfree(kernbuf);
        return 0;
    }
};

//Handle read using copyout to copy using getch to temp and copy to user buffer from temp (checks validity of user buf ptr) if successful (else return with error)
int sys_read(void* buf, size_t nbytes) {
    char* kernbuf = kmalloc(sizeof(char)*nbytes);
    if (kernbuf == NULL) return ENOMEM;
    
    size_t i;
    for(i = 0; i < nbytes; i++) {
        char temp = getch();
        *(kernbuf + i) = temp;
    }
    
    int failure = copyout(kernbuf, buf, nbytes);

    kfree(kernbuf);
    return (failure ? failure : 0);
};

int sys_fork(trapframeptr currentTF, int*retval) { 
    //Create child TF and copy current TF
    trapframeptr childTF = kmalloc(sizeof(Trapframe));
    if (childTF == NULL) {
        kfree(childTF);
        return ENOMEM;
    }
    memcpy(childTF,currentTF,sizeof(Trapframe));

    //Copy child AS from parent (Need as_copy(curAS, AS** newAS) from addrspace.h)
    addrspaceptr childAS = NULL; 
    if (as_copy(curthread->t_vmspace,&childAS)) {
        kfree(childTF);
        return ENOMEM; //as_copy will only error with ENOMEM else returns 0
    }

    //Now we have child TF and AS... (remember not to mem leak in md_forkentry)
    
    //Create a new threadptr fot the child
    threadptr childThread;

    //Call thread_fork
    int forkerror = thread_fork(curthread->t_name, childTF, (unsigned long) childAS, md_forkentry, &childThread);

    //Only parent gets here (childThread goes to md_forkentry). If no errors, set retval to child's PID and return 0 for no error
    if (forkerror) {
        //kfree?
        //kfree(childTF);
        return forkerror;
    }
    *retval = childThread->pid;
    return 0;
};

int sys_execv(const char* progname, char** args, int* retval) {
    //Initializations
    int result;
    int nargs = 0;
    int index = 0;
    
    //Pointer NULL checking
    if (progname == NULL || args == NULL) {
        *retval = -1;
        return EFAULT;
    }

    //Check for issues with progname ptr
    char* check1 = kmalloc(sizeof(char));
    result = copyin(progname, check1, sizeof(char));
    kfree(check1);
    if (result) {
        return EFAULT;
    }

    //Check if the args is an invalid pointer
    char** check2 = kmalloc(sizeof(char *));
    result = copyin(args, check1, sizeof(char *));
    kfree(check2);
    if (result) {
        return EFAULT;
    }

    //Find how many args there are
    while(args[index] != NULL) {
        nargs++;
        index++;
    }

    //Check if args ptrs are valid by checking first char in str with copyin
    int j;
    char* tempC = kmalloc(sizeof(char));
    for (j = 0; j < nargs; j++) {
        result = copyin(args[j], tempC, sizeof(char));
        if (result) {
            kfree(tempC);
            return EFAULT;
        }
    }
    kfree(tempC);

    //progname error checking
    //Check for empty string
    if (!strlen(progname)) {
        return EINVAL; 
    }
    //Check for progname being too long
    if(strlen(progname) > PATH_MAX) {
        *retval = -1;
        return E2BIG;
    }
    //Done error checking

    //Create a variable to hold the program name and have it exist in user space
    char* kernprog = kmalloc(PATH_MAX);
    char** kernargs;
    result = copyinstr((const_userptr_t)progname, kernprog, strlen(progname) + 1, NULL);
    if(result || kernprog == NULL) {
        kfree(kernprog);
        *retval = -1;
        return result;
    }

    //Put the args into user space and allocate space for the number of arguments
    kernargs = kmalloc(sizeof(char**)*nargs);
    if(kernargs == NULL) {
        kfree(kernprog);
        kfree(kernargs);
        *retval = -1;
        return ENOMEM;
    }

    int i;

    //copy the args to the kernel buffer
    for(i = 0; i < nargs; i++) {
        kernargs[i] = kmalloc(sizeof(char) * (strlen(args[i]) + 1));
        if(kernargs[i] == NULL) {
            *retval = -1;
            //might have to free EVERYTHING in kernargs
            int j = 0;
            for(;j <= i; j++) {
                kfree(kernargs[j]);
            }
            kfree(kernargs);
            kfree(kernprog);
            return ENOMEM;
        }
        result = copyinstr((const_userptr_t)args[i], kernargs[i], strlen(args[i]) + 1, NULL);
        if(result) {
            *retval = -1;
            //kfree(kernargs[i]);
            //might have to free EVERYTHING in kernargs
            int j = 0;
            for(;j <= i; j++) {
                kfree(kernargs[j]);
            }
            kfree(kernargs);
            kfree(kernprog);
            return result;
        }
    }


    //This section is similar to runprogram except changes to vmspace and reading of args
    struct vnode *v;
	vaddr_t entrypoint, stackptr;
	

    /* Open the file. */
    //change this to be the REAL progname
	result = vfs_open(kernprog, O_RDONLY, &v);
	if (result) {
		return result;
	}

    /* We should be a new thread. */
    //Since we are changing programs, this might not be NULL. Make it NULL.
	if(curthread->t_vmspace != NULL) as_destroy(curthread->t_vmspace);

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
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(curthread->t_vmspace, &stackptr);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		return result;
	}

	/*
	 * This is the beginning of oucit declaration of function `splhigh'
../../userprog/oursyscall.c:385: error: `pageTableEntry' undeclared (first use in this function)
../../userprog/oursyscall.c:385: error: (Each undeclared identifier is repr code to deal with arguments
	 */
    
     //Goal: Put args on stack and keep references to them
     //Note: For consistency, want to do things in reverse order as the stack is top to bottom

    //Put the arguments themselves on the stack, and each time fill the args array with the address on the stack
	for(i = (nargs-1); i >= 0;) {
        //Adjust sp (stack ptr) by number of bytes needed for the stack with padding (for alignment)
        stackptr -= (strlen(kernargs[i])  + 1 - (strlen(kernargs[i]) + 1) % 4 + 4);
        
        //Copy current arg to the stack, fail if there is a problem copying, and keep track of mem location
		int fail = copyoutstr(kernargs[i], stackptr, strlen(kernargs[i]) + 1, NULL);
        if(fail) {
            while (i >= 0) {
                kfree(kernargs[i--]);
            }
            
            return fail; //MEMLEAK HERE
        }
        kfree(kernargs[i]); //Don't memory leak (no need to set to NULL to avoid unwanted deref since about to overwrite)
		kernargs[i--] = (char*)stackptr;
	}

    //Put NULL on the space between the args and the addresses (will terminate addresses)
    stackptr -= sizeof(void*); //Mem size of ptr should be 4 bytes (i.e. sp -= 4) but this is more robust
    *((int *)stackptr) = NULL;

    //Put the addresses on the stack (below the args)
	for(i = (nargs - 1); i >= 0; i--) {
        //Mem size of ptr should be 4 bytes (i.e. sp -= 4) but this is more robust
        stackptr -= sizeof(void*);
        
        //Copy current arg to the stack, fail if there is a problem copying, and keep track of mem location
		int fail = copyout(&kernargs[i], stackptr, sizeof(vaddr_t));
		if(fail) return fail;
    }
    
    /*
	 * This is the end of our code to deal with arguments
     * If we come back from user mode we don't really need the last return
     * since panic will kill the OS anyway
	 */

	/* Warp to user mode. */
	md_usermode(nargs, (userptr_t)stackptr,stackptr, entrypoint);
	
	/* md_usermode does not return */
	panic("md_usermode returned\n");
	return EINVAL;
};

//Return the PID of the current thread
int sys_getpid(void) {
    return curthread->pid;
};

int sys_waitpid(pid_t pid, int *status, int options, int* retval) {
    if (pid > 99) {
        return EINVAL;
    }

    //Check status arg (badbeef, deadbeef, NULL, alignment)
    if (status == NULL || status == (int*)0xbadbeef || status == (int*)0xdeadbeef || ((intptr_t)status%4)) {
        return EFAULT;
    }

    //Check options is 0 or special number for menu and if pid is in range and in use
    if((options != 0 && options != 6969) || processtable[pid].pid<=0 || processtable[pid].pidUsed == 0)
        return EINVAL;

    //If not called from the kernel (i.e. from menu) using a special option code, check mem is from user space
    if (options != 6969) {
        int* check = kmalloc(sizeof(int));
        int result = copyin(status, check, sizeof(int));
        kfree(check);
        if (result) return EFAULT;
    }
    
    //Check if someone is already waiting, if so EINVAL else set flag to show waiting
    if (processtable[pid].waited == 1)
        return EINVAL;
    else processtable[pid].waited = 1;

    //Grab lock (process will sleep here if lock not already released and naturally wait for child exit)
    lock_acquire(processtable[pid].exitlock);
    *retval = pid;
    *status = processtable[pid].exit_code;
    lock_release(processtable[pid].exitlock);

    return 0;
};

//Exit by setting exit code, changing pidUsed, giving up the lock (giving it to those waiting to acquire it), and calling thread_exit
int sys__exit(int exitcode) {
    processtable[curthread->pid].exit_code = exitcode;
    processtable[curthread->pid].pidUsed = 1;
    lock_release(processtable[curthread->pid].exitlock);
    //lock_destroy(processtable[curthread->pid].exitlock);
    thread_exit();
    return 0;
};

int sys__time(time_t* secs, unsigned long* nsecs, int* retval) {    
    //Get the time
    time_t* kernsecs = kmalloc(sizeof(time_t));
    u_int32_t* kernnanosecs = kmalloc(sizeof(u_int32_t));
    if (kernsecs==NULL || kernnanosecs==NULL) return ENOMEM;
    gettime(kernsecs, kernnanosecs);

    //copyout the time into secs and nsecs using SC evaluation (includes validity checks)
    *retval = *kernsecs;
    if (secs!=NULL && copyout(kernsecs, secs, sizeof(time_t))) {
            kfree(kernsecs);
            kfree(kernnanosecs);
            return EFAULT;
    }
    if (nsecs!=NULL && copyout(kernnanosecs, nsecs, sizeof(u_int32_t))) {
            kfree(kernsecs);
            kfree(kernnanosecs);
            return EFAULT;
    }
    
    //Don't leak memory
    kfree(kernsecs);
    kfree(kernnanosecs);
    return 0;
}

int sys_sbrk(intptr_t amount, int* retval) {
    // kprintf("sbrk called with %d", amount);
    //Amount is 0, no increase and give back old end
    if (amount == 0) {
        *retval = ((curthread->t_vmspace)->heap).vend;
        return 0;
    }
    
    //Check that amount is valid increase param (page alignment)
    if (amount % 4) {
        *retval = ((void *)-1);
        return EINVAL;
    }

    //Set return value to be the amount
    *retval = ((curthread->t_vmspace)->heap).vend;

    //Check amount is positive or negative
    if (amount < 0) {
        //Check we don't go off the end of the heap
        if (*retval - amount < ((curthread->t_vmspace)->heap).vbase) {
            *retval = ((void *)-1);
            return EINVAL;
        }

        //Give back any necessary pages
	    int spl;
	    spl = splhigh();

        //Adjust heap end
        ((curthread->t_vmspace)->heap).vend += amount;

        PageTableEntry* current = ((curthread->t_vmspace)->heap).localPageTable;
        if (current == NULL) {
            splx(spl);
            return 0;
        }
        
        //Check if the first entry needs to be freed
        if ((current != NULL) && (current->vaddr >= ((curthread->t_vmspace)->heap).vend)) {
            ((curthread->t_vmspace)->heap).localPageTable = NULL;
            int cmIndex = (current->paddr - firstpaddr)/PAGE_SIZE;
		    ourcoremap[cmIndex].state--; //state-- rather than CM_FREE
            kfree(current);
            splx(spl);
            ((curthread->t_vmspace)->heap).numPages--;
            return 0;
        }

        //Otherwise, start scan until the end
        PageTableEntry* prev = current;
        current = current->next;
        while (current != NULL) {
            if (current->vaddr >= ((curthread->t_vmspace)->heap).vend) {
                //Rethread
                prev->next = current->next;
                //Free page and reset current
                int cmIndex = (current->paddr - firstpaddr)/PAGE_SIZE;
		        ourcoremap[cmIndex].state--; //state-- rather than CM_FREE
                kfree(current);
                current = prev->next;
                ((curthread->t_vmspace)->heap).numPages--;
            }
            else {
                prev = current;
                current = current -> next;
            }
        }
        splx(spl);
        return 0;
    }
    else {
        //Check that you're not running into the end of the allowable heap size
        if ((*retval + amount) >= (((curthread->t_vmspace)->heap).vbase + HEAPLIMIT)) {
            *retval = ((void *)-1);
            return ENOMEM;
        }
    }

    //Adjust the heap end
    ((curthread->t_vmspace)->heap).vend += amount;

    return 0;
}
