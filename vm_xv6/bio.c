// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to flush it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.
//
// The implementation uses three state flags internally:
// * B_BUSY: the block has been returned from bread
//     and has not been passed back to brelse.
// * B_VALID: the buffer data has been initialized
//     with the associated disk block contents.
// * B_DIRTY: the buffer data has been modified
//     and needs to be written to disk.

#include "types.h"
#include "defs.h"
#include "param.h"
//#include "spinlock.h"
#include "buf.h"
#include "xv6.h"

void __test_diskcache();

struct
{
    // struct spinlock lock;
    struct buf buf[NBUF];

    // Linked list of all buffers, through prev/next.
    // head.next is most recently used.
    struct buf head;
} bcache;

void binit(void)
{	
	PRINTC("(binit) - disk cache\n");

    struct buf *b;

    //LOCK_INIT();

    //PAGEBREAK!
    // Create linked list of buffers
    bcache.head.prev = &bcache.head;
    bcache.head.next = &bcache.head;
    for (b = bcache.buf; b < bcache.buf + NBUF; b++)
    {
		b->next = bcache.head.next;
		b->prev = &bcache.head;
		b->dev = -1;
		bcache.head.next->prev = b;
		bcache.head.next = b;
    }

	__test_diskcache();
}

// Look through buffer cache for sector on device dev.
// If not found, allocate fresh block.
// In either case, return locked buffer.
static struct buf *
bget(uint dev, uint sector)
{
    struct buf *b;

//LOCK_TAKE();

loop:
    // Try for cached block.
    for (b = bcache.head.next; b != &bcache.head; b = b->next)
    {
		if (b->dev == dev && b->sector == sector)
		{
			if (!(b->flags & B_BUSY))
			{
				b->flags |= B_BUSY;
				//LOCK_RELEASE();
				return b;
			}
			//SLEEP();
			goto loop;
		}
    }

    // Allocate fresh block.
    for (b = bcache.head.prev; b != &bcache.head; b = b->prev)
    {
		if ((b->flags & B_BUSY) == 0)
		{
			b->dev = dev;
			b->sector = sector;
			b->flags = B_BUSY;
			//LOCK_RELEASE();
			return b;
		}
    }

    /* ethan: 
   * will NEVER reach here, in theroy
   */
    PRINTC_ERR("bget: out of buffers.\n");

    return (struct buf *)0;
}

// Return a B_BUSY buf with the contents of the indicated disk sector.
struct buf *
bread(uint dev, uint sector)
{
    struct buf *b;

    b = bget(dev, sector);
    if (!(b->flags & B_VALID))
		iderw(b);
    return b;
}

// Write b's contents to disk.  Must be locked.
void bwrite(struct buf *b)
{
    if ((b->flags & B_BUSY) == 0)
		PRINTC_ERR("bwrite");
    b->flags |= B_DIRTY;
    iderw(b);
}

// Release the buffer b.
void brelse(struct buf *b)
{
    if ((b->flags & B_BUSY) == 0)
		PRINTC_ERR("brelse");

    //LOCK_TAKE();

    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;

    b->flags &= ~B_BUSY;
    //?? wakeup(b);

    //LOCK_RELEASE();
}

void
__test_diskcache()
{
	PRINTC("\tTest:\n");
	
	struct buf *b, *t;
	b = bread(0,0);
	PRINTC("\t\t[1]Read sector[0] through disk cache, flags:0x%x, Data:%s\n",b->flags,b->data);
	
	xv6_memmove(b->data,"SUCCESS",8);

	bwrite(b);
	brelse(b);
    PRINTC("\t\t[2]Write to disk & Release this cache.\n");

	t = bread(0,0);
	PRINTC("\t\t[3]Read sector[0] through disk cache, flags:0x%x, Data:%s\n",t->flags, t->data);
	
	brelse(t);

	PRINTC("\tTest Done.\n");

}
