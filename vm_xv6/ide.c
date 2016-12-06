// // Simple PIO-based (non-DMA) IDE driver code.

#include "types.h"
#include "defs.h"
#include "param.h"
//#include "mmu.h"
//#include "proc.h"
#include "x86.h"
//#include "traps.h"
//#include "spinlock.h"
#include "buf.h"
#include "xv6.h"

// #define IDE_BSY       0x80
// #define IDE_DRDY      0x40
// #define IDE_DF        0x20
// #define IDE_ERR       0x01

// #define IDE_CMD_READ  0x20
// #define IDE_CMD_WRITE 0x30

// // idequeue points to the buf now being read/written to the disk.
// // idequeue->qnext points to the next buf to be processed.
// // You must hold idelock while manipulating queue.

// static struct spinlock idelock;
// static struct buf *idequeue;

// static int havedisk1;
// static void idestart(struct buf*);

// // Wait for IDE disk to become ready.
// static int
// idewait(int checkerr)
// {
//   int r;

//   while(((r = inb(0x1f7)) & (IDE_BSY|IDE_DRDY)) != IDE_DRDY)
//     ;
//   if(checkerr && (r & (IDE_DF|IDE_ERR)) != 0)
//     return -1;
//   return 0;
// }

void ideinit(void)
{
    diskinit();
    //   int i;

    //   initlock(&idelock, "ide");
    //   picenable(IRQ_IDE);
    //   ioapicenable(IRQ_IDE, ncpu - 1);
    //   idewait(0);

    //   // Check if disk 1 is present
    //   outb(0x1f6, 0xe0 | (1<<4));
    //   for(i=0; i<1000; i++){
    //     if(inb(0x1f7) != 0){
    //       havedisk1 = 1;
    //       break;
    //     }
}

//   // Switch back to disk 0.
//   outb(0x1f6, 0xe0 | (0<<4));
// }

// // Start the request for b.  Caller must hold idelock.
// static void
// idestart(struct buf *b)
// {
//   if(b == 0)
//     panic("idestart");

//   idewait(0);
//   outb(0x3f6, 0);  // generate interrupt
//   outb(0x1f2, 1);  // number of sectors
//   outb(0x1f3, b->sector & 0xff);
//   outb(0x1f4, (b->sector >> 8) & 0xff);
//   outb(0x1f5, (b->sector >> 16) & 0xff);
//   outb(0x1f6, 0xe0 | ((b->dev&1)<<4) | ((b->sector>>24)&0x0f));
//   if(b->flags & B_DIRTY){
//     outb(0x1f7, IDE_CMD_WRITE);
//     outsl(0x1f0, b->data, 512/4);
//   } else {
//     outb(0x1f7, IDE_CMD_READ);
//   }
// }

// // Interrupt handler.
// void
// ideintr(void)
// {
//   struct buf *b;

//   // Take first buffer off queue.
//   acquire(&idelock);
//   if((b = idequeue) == 0){
//     release(&idelock);
//     cprintf("Spurious IDE interrupt.\n");
//     return;
//   }
//   idequeue = b->qnext;

//   // Read data if needed.
//   if(!(b->flags & B_DIRTY) && idewait(1) >= 0)
//     insl(0x1f0, b->data, 512/4);

//   // Wake process waiting for this buf.
//   b->flags |= B_VALID;
//   b->flags &= ~B_DIRTY;
//   wakeup(b);

//   // Start disk on next buf in queue.
//   if(idequeue != 0)
//     idestart(idequeue);

//   release(&idelock);
// }

//PAGEBREAK!
// Sync buf with disk.
// If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
// Else if B_VALID is not set, read buf from disk, set B_VALID.
void iderw(struct buf *b)
{
    /* ethan:
 	 * rewrite 
 	 */
    if (!(b->flags & B_BUSY))
		PRINTC_ERR("iderw: buf not busy");
    if ((b->flags & (B_VALID | B_DIRTY)) == B_VALID)
		PRINTC_ERR("iderw: nothing to do");
	
	/* ethan:
	 * check if D_DIRTY is set
	 * B_DIRTY = 100, | with 011, if B_DIRTY is set, the result should
	 * be 111, otherwise, the result is 011
	 */
	if((b->flags|0x3) == 0x7)
	{
		diskwrite(b,b->dev,b->sector);

		/* set B_VALID */
		b->flags = (b->flags | 0x2);
	}

	/* ethan:
	 * check if B_VALID is set
	 * B_VALID = 010, | with 101, if B_VALID is set, the result should
	 * be 111, otherwise, the result is 101
	 */
	 if((b->flags|0x5) == 0x5)
	 {
		 diskread(b,b->dev,b->sector);

		 /* set B_VALID */
		 b->flags = (b->flags | 0x2);
	 }

}
