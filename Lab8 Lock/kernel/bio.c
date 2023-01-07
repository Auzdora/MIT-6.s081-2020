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

struct {
  // struct spinlock lock;
  struct buf buf[NBUF];
  struct spinlock bclock[NBUCKET];
  struct buf bucket[NBUCKET];
  struct spinlock movelock;
  
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  // struct buf head;
} bcache;

uint hash(uint blockno) {
  return blockno % NBUCKET;
}

void
binit(void)
{
  struct buf *b;
  initlock(&bcache.movelock, "movelock");
  
  for(int i=0; i < NBUCKET; i++) {
    initlock(&bcache.bclock[i], "bcache");
    bcache.bucket[i].next = 0;
  }

  for(b = bcache.buf; b < bcache.buf + NBUF; b++) {
    b->tstamp = 0;
    b->refcnt = 0;
    b->next = bcache.bucket[0].next;
    initsleeplock(&b->lock, "buffer");
    bcache.bucket[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  uint n = hash(blockno);
  acquire(&bcache.bclock[n]);

  // Is the block already cached?
  for(b = bcache.bucket[n].next; b; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bclock[n]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  release(&bcache.bclock[n]);
  acquire(&bcache.movelock);
  
  for(b = bcache.bucket[n].next; b; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      acquire(&bcache.bclock[n]);
      b->refcnt++;
      release(&bcache.bclock[n]);
      release(&bcache.movelock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // search global minimum time stamp buf
  struct buf *LRUbuf = 0;
  struct buf *prevLRUbuf = 0;
  int current_no = 0;
  uint find = 0;

  for(int i=0; i < NBUCKET; i++) {
    acquire(&bcache.bclock[i]);
    find = 0;
    for(b = &bcache.bucket[i]; b->next; b = b->next){
      if(!prevLRUbuf && b->next->refcnt == 0) prevLRUbuf = b;
      if((prevLRUbuf->next->tstamp > b->next->tstamp) && b->next->refcnt == 0) {
        find = 1;
        prevLRUbuf = b;
      }
    }
    if(find == 1 && i!= 0) {
      release(&bcache.bclock[current_no]);
      current_no = i;
      continue;
    }
    if(find == 0 && i != 0) {
      release(&bcache.bclock[i]);
    }

  }
  if(!prevLRUbuf) panic("bget: no buffers");
  LRUbuf = prevLRUbuf->next;
  if(current_no != n) {
    prevLRUbuf->next = LRUbuf->next;
    release(&bcache.bclock[current_no]);
    acquire(&bcache.bclock[n]);
    LRUbuf->next = bcache.bucket[n].next;
    bcache.bucket[n].next = LRUbuf;
  }
  LRUbuf->blockno = blockno;
  LRUbuf->dev = dev;
  LRUbuf->valid = 0;
  LRUbuf->refcnt = 1;
  release(&bcache.bclock[n]);
  release(&bcache.movelock);
  acquiresleep(&LRUbuf->lock);
  return LRUbuf; 
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  // printf("call bread\n");
  struct buf *b;

  b = bget(dev, blockno);
  // printf("bget's addr is: %p\n",b);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  // printf("end bread\n");
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  // printf("call bwrite\n");
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  // printf("end bwrite\n");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  // printf("b's addr is: %p\n", b);
  // printf("b's blockno: %s, time stamp is: %d\n", b->blockno, b->tstamp);
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&bcache.bclock[hash(b->blockno)]);
  b->refcnt--;
  if(b->refcnt == 0) {
    b->tstamp = ticks;
  }

  release(&bcache.bclock[hash(b->blockno)]);

  // acquire(&bcache.lock);
  // b->refcnt--;
  // if (b->refcnt == 0) {
  //   // no one is waiting for it.
  //   b->next->prev = b->prev;
  //   b->prev->next = b->next;
  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;
  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  // }
  
  // release(&bcache.lock);
}

void
bpin(struct buf *b) {
  acquire(&bcache.bclock[hash(b->blockno)]);
  b->refcnt++;
  release(&bcache.bclock[hash(b->blockno)]);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.bclock[hash(b->blockno)]);
  b->refcnt--;
  release(&bcache.bclock[hash(b->blockno)]);
}


