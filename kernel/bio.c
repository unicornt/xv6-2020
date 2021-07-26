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

#define NHASHBUF 13

struct {
  struct spinlock lock[NHASHBUF];
  struct spinlock superlock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[NHASHBUF];
} bcache;

char lockname[NHASHBUF][9];
void
binit(void)
{
  struct buf *b;

  initlock(&bcache.superlock, "superlock");
  // Create linked list of buffers
  for(int i = 0; i < NHASHBUF; i++) {
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
    snprintf(lockname[i], sizeof(lockname[i]), "bcache%d\0", i);
    initlock(&bcache.lock[i], lockname[i]);
  }
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->timestap = 0;
    b->next = bcache.head[0].next;
    b->prev = &bcache.head[0];
    initsleeplock(&b->lock, "buffer");
    bcache.head[0].next->prev = b;
    bcache.head[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  struct buf *bi;

  int hashid = blockno % NHASHBUF;
  acquire(&bcache.lock[hashid]);

  // Is the block already cached?
  for(b = bcache.head[hashid].next; b != &bcache.head[hashid]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      // b->timestap = ticks;
      release(&bcache.lock[hashid]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  int minstamp = __INT_MAX__;
  b = 0;
  for(bi = bcache.head[hashid].next; bi != &bcache.head[hashid]; bi = bi->next){
    if(bi->refcnt == 0 && bi->timestap < minstamp) {
      minstamp = bi->timestap;
      b = bi;
    }
  }
  if(minstamp != __INT_MAX__){
    goto find;
  }
  acquire(&bcache.superlock);
refind:
  // printf("refind\n");
  minstamp = __INT_MAX__;
  b = 0;
  for(bi = bcache.buf; bi < bcache.buf + NBUF; bi++) {
    if(bi->refcnt == 0 && bi->timestap < minstamp) {
      b = bi;
      minstamp = bi->timestap;
    }
  }
  if(minstamp == __INT_MAX__){
    panic("no buffer");
  }
  // printf("blockno %d %p\n", b->blockno, b);
  int hid2 = b->blockno % NHASHBUF;
  acquire(&bcache.lock[hid2]);
  if(b->refcnt != 0) {
    release(&bcache.lock[hid2]);
    goto refind;
  }
  b->next->prev = b->prev;
  b->prev->next = b->next;
  b->next = bcache.head[hashid].next;
  b->prev = &bcache.head[hashid];
  bcache.head[hashid].next->prev = b;
  bcache.head[hashid].next = b;
  release(&bcache.lock[hid2]);
  release(&bcache.superlock);
find:
  // printf("find\n");
  // b->timestap = ticks;
  b->valid = 0;
  b->blockno = blockno;
  b->dev = dev;
  b->refcnt = 1;
  release(&bcache.lock[hashid]);
  acquiresleep(&b->lock);
  return b;
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

  int hashid = b->blockno % NHASHBUF;

  acquire(&bcache.lock[hashid]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->timestap = ticks;
    // b->next->prev = b->prev;
    // b->prev->next = b->next;
    // b->next = bcache.head[hashid].next;
    // b->prev = &bcache.head[hashid];
    // bcache.head[hashid].next->prev = b;
    // bcache.head[hashid].next = b;
  }
  release(&bcache.lock[hashid]);
}

void
bpin(struct buf *b) {
  int hashid = b->blockno % NHASHBUF;
  acquire(&bcache.lock[hashid]);
  b->refcnt++;
  release(&bcache.lock[hashid]);
}

void
bunpin(struct buf *b) {
  int hashid = b->blockno % NHASHBUF;
  acquire(&bcache.lock[hashid]);
  b->refcnt--;
  release(&bcache.lock[hashid]);
}


