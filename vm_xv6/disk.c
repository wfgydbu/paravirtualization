#include "xv6.h"
#include "types.h"
#include "param.h"
#include "defs.h"
#include "buf.h"
#include "fs.h"

#define SECTORS_NUM (DISKSIZE / SECTORSIZE)

char disk[DISKSIZE];

void __test_disk();

void diskinit(void)
{
    PRINTC("(diskinit) - disk\n");

    xv6_memset(disk, 0, DISKSIZE);
    PRINTC("\tVirtual Disk is ready: %d sectors, each 512 bytes, %d MB totally\n", SECTORS_NUM, DISKSIZE / 1024 / 1024);

	/* ethan: 
	 * set up super blcok in sector 1
	 * cheat by assuming 1024 sector for logging, 10247 sectors for inodes.
	 * 13311 sectors for bitmap and 106488 sectors for data.
	 */ 
	struct superblock sb;

	sb.size = SECTORS_NUM;
	sb.nblocks = 106488;
    sb.ninodes = 10247;

	struct buf kbuf;
	xv6_memmove(kbuf.data, &sb, sizeof(struct superblock));

    diskwrite(&kbuf, 0, 1);
	PRINTC("\tSet up Superblock: %d data blocks and %d inodes.\n", sb.nblocks, sb.ninodes);

    __test_disk();
    return;
}

/* ethan:
 * read a sector(512byte) from a disk
 * here, we cheat by only one disk
 * so uint dev makes no sense.
 */
void diskread(struct buf *b, uint dev, uint sector)
{
    xv6_memset(b->data, 0, SECTORSIZE);

    xv6_memmove(b->data, (char *)&disk[SECTORSIZE * sector], SECTORSIZE);

    return;
}

/* ethan: again, ingore dev for now */
void diskwrite(struct buf *b, uint dev, uint sector)
{
    xv6_memmove(&disk[SECTORSIZE * sector], b->data, SECTORSIZE);

    return;
}

/* ethan: test disk */
void __test_disk()
{
    PRINTC("\tTest:\n");

    struct buf kbuf, pbuf;

    xv6_memset(kbuf.data, 0, 512);
    xv6_memset(pbuf.data, 0, 512);
    xv6_memmove(kbuf.data, "SUCCESS", 8);

    diskwrite(&kbuf, 0, 0);
    diskread(&pbuf, 0, 0);
    PRINTC("\t\tWrite and Read data to/from sector 0:      %s\n", pbuf.data);

    diskwrite(&kbuf, 0, 512);
    diskread(&pbuf, 0, 512);
    PRINTC("\t\tWrite and Read data to/from sector 512:    %s\n", pbuf.data);

    diskwrite(&kbuf, 0, 1024);
    diskread(&pbuf, 0, 1024);
    PRINTC("\t\tWrite and Read data to/from sector 1024:   %s\n", pbuf.data);

    diskwrite(&kbuf, 0, 131071);
    diskread(&pbuf, 0, 131071);
    PRINTC("\t\tWrite and Read data to/from sector 131071: %s\n", pbuf.data);

    //xv6_memset(disk, 0, DISKSIZE);
    PRINTC("\tTest Done.\n");
}