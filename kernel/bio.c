// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKETS 13

struct {
  struct spinlock lock[NBUCKETS];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
  struct buf hashbucket[NBUCKETS];
} bcache;

void
binit(void)
{
  struct buf *b;

  for(int i = 0; i < NBUCKETS; i++){
    initlock(&bcache.lock[i], "bcache");
    // Create linked list of buffers
    bcache.hashbucket[i].prev = &bcache.hashbucket[i];
    bcache.hashbucket[i].next = &bcache.hashbucket[i];
  }
  int block = 0;
  for(b = bcache.buf; b < bcache.buf+NBUF;b++, block++){
    b->next = bcache.hashbucket[block % NBUCKETS].next;
    b->prev = &bcache.hashbucket[block % NBUCKETS];
    initsleeplock(&b->lock, "buffer");
    bcache.hashbucket[block % NBUCKETS].next->prev = b;
    bcache.hashbucket[block % NBUCKETS].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int block = blockno % NBUCKETS;
  acquire(&bcache.lock[block]);

  // Is the block already cached?
  for(b = bcache.hashbucket[block].next; b != &bcache.hashbucket[block]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[blockno % NBUCKETS]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached; recycle an unused buffer.
  for(b = bcache.hashbucket[block].prev; b != &bcache.hashbucket[block]; b = b->prev)
  {
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[blockno % NBUCKETS]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  while(1){
    block = (block + 1) % NBUCKETS;
    if(block == blockno % NBUCKETS){
      break;
    }
    acquire(&bcache.lock[block]);
    for(b = bcache.hashbucket[block].prev; b != &bcache.hashbucket[block]; b = b->prev)
    {
      if(b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        
        // remove from the list
        b->next->prev = b->prev;
        b->prev->next = b->next;
        release(&bcache.lock[block]);
        
        b->next = bcache.hashbucket[blockno % NBUCKETS].next;
        b->prev = &bcache.hashbucket[blockno % NBUCKETS];
        bcache.hashbucket[blockno % NBUCKETS].next->prev = b;
        bcache.hashbucket[blockno % NBUCKETS].next = b;
        release(&bcache.lock[blockno % NBUCKETS]);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.lock[block]);
  }
  

  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b->dev, b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b->dev, b, 1);
}

// Release a locked buffer.
// Move to the head of the MRU list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int block = b->blockno % NBUCKETS;
  acquire(&bcache.lock[block]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.hashbucket[block].next;
    b->prev = &bcache.hashbucket[block];
    bcache.hashbucket[block].next->prev = b;
    bcache.hashbucket[block].next = b;
  }
  
  release(&bcache.lock[b->blockno % NBUCKETS]);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock[b->blockno % NBUCKETS]);
  b->refcnt++;
  release(&bcache.lock[b->blockno % NBUCKETS]);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock[b->blockno % NBUCKETS]);
  b->refcnt--;
  release(&bcache.lock[b->blockno % NBUCKETS]);
}


