#ifndef _VM_H_
#define _VM_H_

#include <machine/vm.h>


/*
 * VM system-related definitions.
 *
 * You'll probably want to add stuff here.
 */

//Kernel to virtual address mapping (courtesy of mips vm.h)
#define KVADDR_TO_PADDR(vaddr) ((vaddr)-MIPS_KSEG0)

//Coremap states
#define CM_FREE 0
#define CM_USED 1

/* Fault-type arguments to vm_fault() */
#define VM_FAULT_READ        0    /* A read was attempted */
#define VM_FAULT_WRITE       1    /* A write was attempted */
#define VM_FAULT_READONLY    2    /* A write to a readonly page was attempted*/

/* Initialization function */
void vm_bootstrap(void);

/* Fault handling function called by trap code */
int vm_fault(int faulttype, vaddr_t faultaddress);

/* Allocate/free kernel heap pages (called by kmalloc/kfree) */
vaddr_t alloc_kpages(int npages);
void free_kpages(vaddr_t addr);
paddr_t getppages(unsigned long npages);

//Coremap element structure
typedef struct coremap_entry {
    //vaddr_t virtualaddr;
    //int timesReferenced;
    int state;
    int pagesRemaining;
 } Coremap_entry;

//Comparison helper func
inline int betweenVals(vaddr_t comp, vaddr_t val1, vaddr_t val2);

#endif /* _VM_H_ */
