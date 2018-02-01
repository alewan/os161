#ifndef _OUREXTERN_H_
#define _OUREXTERN_H_

#include <vm.h>
#include <thread.h>

#define COREMAP_SIZE 100
#define PROCESSTABLE_SIZE 100

extern int currentpidcount;
extern Process processtable[PROCESSTABLE_SIZE];
extern u_int32_t firstpaddr;
extern int totalpages;
extern Coremap_entry ourcoremap[COREMAP_SIZE];

#endif /* _OUREXTERN_H_ */
