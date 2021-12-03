#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

#define base (KERNBASE - (64 * PGSIZE))

struct {
  struct spinlock lock;
  struct shm_page {
    uint id;
    char *frame;
    int refcnt;
  } shm_pages[64];
} shm_table;

void shminit() {
  int i;
  initlock(&(shm_table.lock), "SHM lock");
  acquire(&(shm_table.lock));
  for (i = 0; i< 64; i++) {
    shm_table.shm_pages[i].id =0;
    shm_table.shm_pages[i].frame =0;
    shm_table.shm_pages[i].refcnt =0;
  }
  release(&(shm_table.lock));
}

int shm_open(int id, char **pointer) {

  //counter for loops
  int i, tf;
  struct proc *curproc = myproc();
  //acquires shared page table lock so it can be looked at and modified without worry.
  acquire(&(shm_table.lock));
  //case 1, the id exists in the table, so no need to alloc memory
  tf=0;
  for (i = 0; i< 64; i++) {
    //found page since id exists
    if(shm_table.shm_pages[i].id == id){
      //mapping the page to the calling process so it can access the page and modify it
      mappages(curproc->pgdir, (void*)PGROUNDUP(curproc->sz), PGSIZE, V2P(shm_table.shm_pages[i].frame),PTE_W|PTE_U);
      //mapping the sent pointer to the shared page's virtual address so it can be displayed.
      *pointer = (char*)PGROUNDUP(curproc->sz);
      //increasing the calling proc's memory size to account for shared page
      curproc->sz += PGSIZE;
      //increment ref
      shm_table.shm_pages[i].refcnt +=1;
      //can break out of loop if block was found, also sets the tf test to true and wont run the following for loop for case two
      tf=1;
      break;
    }
  }
  //case 2, if the id does not exist
  if(!tf){
  for (i = 0; i< 64; i++) {
    //finds the first page in the table with an empty id, signalling an empty shared page
    if(shm_table.shm_pages[i].id == 0){
      //empty entry, so we can alloc
      shm_table.shm_pages[i].frame =kalloc();
      //prints an error statement if kalloc failed
      if(!(shm_table.shm_pages[i].frame)){
        cprintf("ERROR, pg could not be alloced");
        return 0;
      }
      //fills the allocated page with stuff
      memset(shm_table.shm_pages[i].frame,0,PGSIZE);
      //setting the values of the shared page table entry to signal it is accessable and read/writable
      shm_table.shm_pages[i].id = id;
      shm_table.shm_pages[i].refcnt +=1;
      //mapping the page to the calling process so it can access the page and modify it
      mappages(curproc->pgdir, (void*)PGROUNDUP(curproc->sz), PGSIZE, V2P(shm_table.shm_pages[i].frame),PTE_W|PTE_U);
      //mapping the sent pointer to the shared page's virtual address so it can be displayed.
      *pointer = (char*)PGROUNDUP(curproc->sz);
      //increasing the calling proc's memory size to account for shared page
      curproc->sz += PGSIZE;
      //can break out of loop since block was found
      break;
    }
  }
  }
  //releases lock so other processes can access the shared page table
  release(&(shm_table.lock));
return 0; //added to remove compiler warning -- you should decide what to return
}


int shm_close(int id) {
//you write this too!

  int i;
  acquire(&(shm_table.lock));
  for (i = 0; i< 64; i++) { 
    if (shm_table.shm_pages[i].id == id) { // if page is found
      if (shm_table.shm_pages[i].refcnt > 0) { // decrements reference count if count is nonzero
        shm_table.shm_pages[i].refcnt-=1;
      }
      if (shm_table.shm_pages[i].refcnt == 0) { // clears table if count reaches zero
        shm_table.shm_pages[i].frame = 0;
        shm_table.shm_pages[i].id = 0;
        break;
      }
  	}
  }
  release(&(shm_table.lock));
  
return 0; //added to remove compiler warning -- you should decide what to return
}
