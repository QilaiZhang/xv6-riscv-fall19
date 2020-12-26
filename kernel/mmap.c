#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "spinlock.h"
#include "proc.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"
#include "mmap.h"

struct VMA *
map_alloc(void){
  struct proc *p = myproc();

  for(int i = 0; i < NVMA; i++){
    if(p->vma[i].valid){
      p->vma[i].valid = 0;
      return &p->vma[i];
    }
  }
  return 0;
}

uint64
sys_mmap(){
  struct proc *p = myproc();
  uint64 addr;
  int length, offset, prot, flags, fd;
  struct VMA *vma;
  struct file *f;

  
  if(argaddr(0, &addr) < 0 || argint(1, &length) < 0 || argint(2, &prot) < 0 || 
     argint(3, &flags) < 0 || argint(4, &fd) < 0 || argint(5, &offset) < 0)
    return -1;

  f = p->ofile[fd];

  if(addr != 0 || offset != 0)
    panic("sys_map: not zero");

  if((vma = map_alloc()) == 0){
    return -1;
  }

  uint64 end = PGROUNDDOWN(p->maxva);
  uint64 start = PGROUNDDOWN(p->maxva - length);
  vma->fd = fd;
  vma->file = f;
  vma->flags = flags;
  vma->prot = prot;
  vma->end = end;
  vma->start = start;
  vma->file->ref++;
  p->maxva = start;

  return start;
}

uint64
sys_munmap(){
  struct proc *p = myproc();
  uint64 addr;
  int length;
  struct VMA *vma = 0;

  if(argaddr(0, &addr) < 0 || argint(1, &length) < 0){
    return -1;
  }

  for(int i = 0; i < NVMA; i++){
    if(p->vma[i].valid == 0 && p->vma[i].start <= addr && addr <= p->vma[i].end){
        vma = &p->vma[i];
        break;
      }
  }

  if(!vma){
    return -1;
  }
  
  if(walkaddr(p->pagetable, vma->start)){
    if(vma->flags == MAP_SHARED){
      filewrite(vma->file, vma->start, length);
    }
    uvmunmap(p->pagetable, vma->start, length ,1);
  }


  vma->start += length;
  if(vma->start == vma->end){
    vma->file->ref--;
    vma->valid = 1;
  }

  return 0;
}