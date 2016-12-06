#define NPROC        64  // maximum number of processes
#define PAGE       4096  // granularity of user-space memory allocation
#define KSTACKSIZE PAGE  // size of per-process kernel stack
#define NCPU          8  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
#define NBUF         10  // size of disk block cache
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       1  // device number of file system root disk
#define SECTORSIZE  512  // size of disk sector(block)
#define DISKSIZE   (64*1024*1024) // size of disk, 64MB
#define SINODES    1026  // start sector of inodes
#define SBITMAP   11273  // start sector of bitmap
#define SDATABLOCKs 24584 // start sector of data blocks
