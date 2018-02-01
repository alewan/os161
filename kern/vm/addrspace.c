#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>
#include <ourextern.h>
#include <machine/tlb.h>

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

inline vaddr_t maximum (vaddr_t a, vaddr_t b) {
	return (a>b) ? a : b;
}

static int createPageTable (Region* region) {
	assert(region->localPageTable == NULL);
	assert(((region->vbase) & PAGE_FRAME) == region->vbase);
	assert(((region->vend) & PAGE_FRAME) == region->vend);
	
	if (region->vbase == region->vend) return 0;
	vaddr_t temp = region->vbase;
	
	PageTableEntry* current = kmalloc(sizeof(PageTableEntry));
	if (current == NULL) return ENOMEM;
	current->vaddr = temp;
	current->paddr = getppages(1);
	if (current->paddr == 0)
		return ENOMEM;
	current->next = NULL;
	region->localPageTable = current;
	temp+= PAGE_SIZE;

	while (temp < region->vend) {
		current->next = kmalloc(sizeof(PageTableEntry));
		current = current->next;
		if (current == NULL) return ENOMEM;
		current->vaddr = temp;
		current->paddr = getppages(1);
		if (current->paddr == 0)
			return ENOMEM;
		current->next = NULL;
		temp+= PAGE_SIZE;
	}
	return 0;
}

static void destroyPageTable (Region* region) {
	int spl;
	spl = splhigh();
	PageTable temp;
	int cmIndex;
	while (region->localPageTable != NULL) {
		temp = region->localPageTable;
		cmIndex = (temp->paddr - firstpaddr)/PAGE_SIZE;
		ourcoremap[cmIndex].state--; //state-- rather than CM_FREE
		region->localPageTable = (region->localPageTable)->next;
		kfree(temp);
	}
	splx(spl);
	return;
}

int as_copy_region(Region* old, Region* new) {
	//Copy the virtual addresses for the regions
	new->vbase = old->vbase;
	new->vend = old->vend;
	new->numPages = old->numPages;

	//Do a deep copy of the localPageTable
	PageTable source = old->localPageTable;
	PageTable target;

	//int spl = splhigh();
	if(source == NULL) target = NULL;
	else target = kmalloc(sizeof(PageTableEntry));
	if(target == NULL && source != NULL) return ENOMEM;

	while(source != NULL) {
		target->vaddr = source->vaddr;
		target->paddr = getppages(1);
		target->next = NULL;

		if (target->paddr == (paddr_t)0) {
			//splx(spl);
			return ENOMEM;
		}
		
		//void *memmove(dest, source const void *, sizesize_t);
		memmove((void*)PADDR_TO_KVADDR(target->paddr), (const void *)PADDR_TO_KVADDR(source->paddr), PAGE_SIZE);
		source = source->next;
		if (source != NULL) {
			target->next = kmalloc(sizeof(PageTableEntry));
			if(target == NULL) {
				///splx(spl);
				return ENOMEM;
			}
		}
	}
	//splx(spl);
	return 0;
}

//Initialize an addr space
struct addrspace* as_create(void) {
	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	if (as==NULL) {
		return NULL;
	}

	//DUMBVM Implementation
	// 	as->as_vbase1 = 0;
	// 	as->as_pbase1 = 0;
	// 	as->as_npages1 = 0;
	// 	as->as_vbase2 = 0;
	// 	as->as_pbase2 = 0;
	// 	as->as_npages2 = 0;
	// 	as->as_stackpbase = 0;

	// 	return as;
	
	/*
	 * Initialize as needed.
	 */
	//OUR Implementation

	(as->region1).vend = (as->region1).vbase = (vaddr_t)0;
	(as->region1).localPageTable = NULL;
	(as->region1).numPages = 0;

	(as->region2).vend = (as->region2).vbase = (vaddr_t)0;
	(as->region2).localPageTable = NULL;
	(as->region2).numPages = 0;

	(as->heap).vend = (as->heap).vbase = (vaddr_t)0;
	(as->heap).localPageTable = NULL;
	(as->heap).numPages = 0;

	(as->stack).vend = (as->stack).vbase = (vaddr_t)0;
	(as->stack).localPageTable = NULL;
	(as->stack).numPages = 0;
	
	return as;
}

int as_copy(struct addrspace *old, struct addrspace **ret) {
	//DUMBVM Implemenation
	// 	struct addrspace *new;

	// 	new = as_create();
	// 	if (new==NULL) {
	// 		return ENOMEM;
	// 	}

	// 	new->as_vbase1 = old->as_vbase1;
	// 	new->as_npages1 = old->as_npages1;
	// 	new->as_vbase2 = old->as_vbase2;
	// 	new->as_npages2 = old->as_npages2;

	// 	if (as_prepare_load(new)) {
	// 		as_destroy(new);
	// 		return ENOMEM;
	// 	}

	// 	assert(new->as_pbase1 != 0);
	// 	assert(new->as_pbase2 != 0);
	// 	assert(new->as_stackpbase != 0);

	// 	memmove((void *)PADDR_TO_KVADDR(new->as_pbase1),
	// 		(const void *)PADDR_TO_KVADDR(old->as_pbase1),
	// 		old->as_npages1*PAGE_SIZE);

	// 	memmove((void *)PADDR_TO_KVADDR(new->as_pbase2),
	// 		(const void *)PADDR_TO_KVADDR(old->as_pbase2),
	// 		old->as_npages2*PAGE_SIZE);

	// 	memmove((void *)PADDR_TO_KVADDR(new->as_stackpbase),
	// 		(const void *)PADDR_TO_KVADDR(old->as_stackpbase),
	// 		DUMBVM_STACKPAGES*PAGE_SIZE);

	// 	*ret = new;
	// 	return 0;

	struct addrspace *newas;

	newas = as_create();
	if (newas==NULL) {
		return ENOMEM;
	}

	/*
	 * Write this.
	 */


	//Copy the regions one by one
	int result = as_copy_region(&(old->region1), &(newas->region1));
	if(result) return result;
	result = as_copy_region(&(old->region2), &(newas->region2));
	if(result) return result;
	result = as_copy_region(&(old->stack), &(newas->stack));
	if(result) return result;
	result = as_copy_region(&(old->heap), &(newas->heap));
	if(result) return result;

	//newas->as_stackpbase = newas->as_stackpbase;
	
	*ret = newas;
	return 0;
}

void as_destroy(struct addrspace *as) {
	/*
	 * Clean up as needed.
	 */

	//TODO: make this atomic
	destroyPageTable(&(as->region1));
	destroyPageTable(&(as->region2));
	destroyPageTable(&(as->heap));
	destroyPageTable(&(as->stack));

	kfree(as);
}

void as_activate(struct addrspace *as) {
	//DUMBVM Implementation
	 	int i, spl;

	 	(void)as;

	 	spl = splhigh();
		
		//NUM_TLB is defined as 64 elsewhere
		for (i=0; i<NUM_TLB; i++) {
			TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
		}

		splx(spl);
	
	/*
	 * Write this.
	 */

	(void)as;  // suppress warning until code gets written
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
		 int readable, int writeable, int executable) {
	/*
	 * Write this.
	 */

	size_t npages; 

	/* Align the region. First, the base... */
	sz += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = sz / PAGE_SIZE;

	//TODO: fix
	//TODO: is the end of the region inclusive? for now write as exclusive
	if((as->region1).vbase == 0) {
		(as->region1).vbase = vaddr;
		(as->region1).vend = vaddr + (npages*PAGE_SIZE);
		(as->region1).numPages = npages;
	}
	else if((as->region2).vbase == 0) {
		(as->region2).vbase = vaddr;
		(as->region2).vend = vaddr + (npages * PAGE_SIZE);
		(as->region2).numPages = npages;	
	}
	return 0;


	/* We don't use these - all pages are read-write */
	(void)readable;
	(void)writeable;
	(void)executable;

// 	if (as->as_vbase1 == 0) {
// 		as->as_vbase1 = vaddr;
// 		as->as_npages1 = npages;
// 		return 0;
// 	}

// 	if (as->as_vbase2 == 0) {
// 		as->as_vbase2 = vaddr;
// 		as->as_npages2 = npages;
// 		return 0;
// 	}

	/*
	 * Support for more than two regions is not available.
	 */
	kprintf("VM: Warning: too many regions\n");
	return EUNIMP;

	// (void)as;
	// (void)vaddr;
	// (void)sz;
	// (void)readable;
	// (void)writeable;
	// (void)executable;
	// return EUNIMP;
}

int as_prepare_load(struct addrspace *as) {
	//DUMB VM Implementation:
	// 	assert(as->as_pbase1 == 0);
	// 	assert(as->as_pbase2 == 0);
	// 	assert(as->as_stackpbase == 0);

	// 	as->as_pbase1 = getppages(as->as_npages1);
	// 	if (as->as_pbase1 == 0) {
	// 		return ENOMEM;
	// 	}

	// 	as->as_pbase2 = getppages(as->as_npages2);
	// 	if (as->as_pbase2 == 0) {
	// 		return ENOMEM;
	// 	}

	// 	as->as_stackpbase = getppages(DUMBVM_STACKPAGES);
	// 	if (as->as_stackpbase == 0) {
	// 		return ENOMEM;
	// 	}

	/*
	 * Write this.
	 */
	//OUR Implementation - NOT NEEDED WITH DEMAND PAGING
	//Make sure all physical addresses are 0 for each page table entry

	//Allocate all mem for region1 and region2. If fails, clear it out
	// if (createPageTable(&(as->region1))) {
	// 	destroyPageTable(&(as->region1));
	// 	return ENOMEM;
	// }

	// if (createPageTable(&(as->region2))) {
	// 	destroyPageTable(&(as->region2));
	// 	return ENOMEM;
	// }

	// (as->stack).vbase = USERSTACK - (STATICSTACKPAGES*PAGE_SIZE);	

	// if (createPageTable(&(as->stack))) {
	// 	destroyPageTable(&(as->stack));
	// 	return ENOMEM;
	// }

	return 0;
}

int as_complete_load(struct addrspace *as) {
	/*
	 * Write this.
	 */
	//Set up the stack
	assert((as->stack).vbase == 0);
	(as->stack).vend = (as->stack).vbase = USERSTACK;
	(as->stack).numPages = 0;
	
	//Look for the maximum address based on the regions list and start the heap there
	(as->heap).vend = (as->heap).vbase = maximum((as->region1).vend, (as->region2).vend);

	return 0;
}

int as_define_stack(struct addrspace *as, vaddr_t *stackptr) {
	//DUMBVM Implementation
		//assert(as->as_stackpbase != 0);
		//*stackptr = USERSTACK;
		//return 0;

	/*
	 * Write this.
	 */

	// (void)as;

	//set virtual stack to 0
	*stackptr = USERSTACK;
	//do as stuff

	
	return 0;
}

