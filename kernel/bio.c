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

#define prime 13

struct {
  //struct spinlock lock;
  struct buf buf[NBUF];
  //struct buf head;
  struct spinlock hash[prime];
  struct buf hd[prime]; 
} bcache;

void
binit(void)
{
  struct buf *b;

  //initlock(&bcache.lock, "bcache");

  for(int i=0;i<prime;i++){
    initlock(&bcache.hash[i],"bcache");
    bcache.hd[i].prev = &bcache.hd[i];
    bcache.hd[i].next = &bcache.hd[i];
  }

  // Create linked list of buffers
  //bcache.head.prev = &bcache.head;
  //bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    int ind=(b-bcache.buf)%prime;
    b->next = bcache.hd[ind].next;
    b->prev = &bcache.hd[ind];
    initsleeplock(&b->lock, "buffer");
    bcache.hd[ind].next->prev = b;
    bcache.hd[ind].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int ind=blockno%prime;
  acquire(&bcache.hash[ind]);
  // Is the block already cached?
  for(b = bcache.hd[ind].next; b != &bcache.hd[ind]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.hash[ind]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.hd[ind].prev; b != &bcache.hd[ind]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.hash[ind]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  for(int i=(ind+1)%prime;i!=ind;i=(i+1)%prime){
    acquire(&bcache.hash[i]);
    for(b = bcache.hd[i].prev; b != &bcache.hd[i]; b = b->prev){
      if(b->refcnt == 0) {
        //remove the buffer
        b->next->prev = b->prev;
        b->prev->next = b->next;
        release(&bcache.hash[i]);
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        //add the buffer
        b->next = bcache.hd[ind].next;
        b->prev = &bcache.hd[ind];
        bcache.hd[ind].next->prev = b;
        bcache.hd[ind].next = b;
        release(&bcache.hash[ind]);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.hash[i]);
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
    virtio_disk_rw(b, 0);
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
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int ind=b->blockno%prime;
  acquire(&bcache.hash[ind]);
  
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.hd[ind].next;
    b->prev = &bcache.hd[ind];
    bcache.hd[ind].next->prev = b;
    bcache.hd[ind].next = b;
  }
  
  release(&bcache.hash[ind]);
}

void
bpin(struct buf *b) {
  int ind=b->blockno%prime;
  acquire(&bcache.hash[ind]);
  b->refcnt++;
  release(&bcache.hash[ind]);
}

void
bunpin(struct buf *b) {
  int ind=b->blockno%prime;
  acquire(&bcache.hash[ind]);
  b->refcnt--;
  release(&bcache.hash[ind]);
}


