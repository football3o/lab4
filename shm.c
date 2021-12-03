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

//you write this
  int i;
  struct proc *curproc = myproc();
  //struct shm_page mypage;
  acquire(&(shm_table.lock));
  //case 1
  for (i = 0; i< 64; i++) {
    if(shm_table.shm_pages[i].id == id){
      //found page since id exists

      mappages(curproc->pgdir, (void*)PGROUNDUP(curproc->sz), PGSIZE, V2P(shm_table.shm_pages[i].frame),PTE_W|PTE_U);
      *pointer = (char*)PGROUNDUP(curproc->sz);
      curproc->sz += PGSIZE;
      //increment ref
      shm_table.shm_pages[i].refcnt +=1;
    }
  }
  //case 2
  for (i = 0; i< 64; i++) {
    if(shm_table.shm_pages[i].id == 0){
      //empty entry, so we can alloc
      shm_table.shm_pages[i].frame =kalloc();
      if(!(shm_table.shm_pages[i].frame)){
        cprintf("ERROR, pg could not be alloced");
        return 0;
      }
      memset(shm_table.shm_pages[i].frame,0,PGSIZE);
      shm_table.shm_pages[i].id = id;
      shm_table.shm_pages[i].refcnt +=1;
      mappages(curproc->pgdir, (void*)PGROUNDUP(curproc->sz), PGSIZE, V2P(shm_table.shm_pages[i].frame),PTE_W|PTE_U);
      *pointer = (char*)PGROUNDUP(curproc->sz);
      curproc->sz += PGSIZE;
    }
  }

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
