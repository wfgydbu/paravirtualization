# Para-virtualized xv6 on  Composite OS
## Goal
The [Composite](https://github.com/gparmer/Composite) Component-based Operating System is a research OS out of the George Washington University (GWU),  and [xv6](https://pdos.csail.mit.edu/6.828/2012/xv6.html) is a simple Unix-like teaching operating system developed by MIT.  Our goal is to make XV6 run on Composite OS by para-virtualization.

## Run 
To run VM_XV6, you should first run Composite OS by following its instructions on GitHub. Or the following instructions will give you a quick start. Note that these instructions are not your only option, We list them here just because we use them on our PC and they work.
```
[1] Use Ubuntu 14.04 32bit.
[2] Install build-essential, binutils-dev, qemu, git.
[3] cd to your directoy, then execute:
	$ git clone git@github.com:gparmer/composite.git -b ppos
	$ cd composite/src/
	$ make config
	$ make init
	$ make ; make cp
	$ cd ../transfer/
	$ sh qemu.sh micro_booter.sh
```
By doing these, you may successfully boot Composite OS. Then to run VM_XV6, you need to copy two things from this project to compositeOS's source code tree.
```
[1] Copy folder vm_xv6 to composite/src/components/implementation/no_interface
[2] Copy script trans/vm_xv6_boot.sh to composite/transfer
[3] cd composite/src, execute make;make cp;
[4] cd composite/transfer, execute qemu.sh vm_xv6_boot_sh.
```

## Back to this project
This section describes its main idea, what we've done so far and what we will do in the future.

### Big picture
Firstly, we create a component in file *xv6.c*, and then initialize it by calling *cos_thd_switch()*, then jump to *xv6_main()*, the structure of *xv6_main()* is exactly same as the *main()* in *main.c* in xv6 source tree. Then we modify each of the function to make them run on Composite OS.

![](https://github.com/wfgydbu/paravirtualization/blob/master/trans/bigpicture.jpg)

### Conventions
To emphasize the concept of virtualization, we made some conventions when we started the project, which are:

1. Keep the structure of xv6 source code, including variable names, function names and file names, only modify them when it's necessary. 
2. Preferentially use functions defined by xv6. For example, both *<string.h>* and xv6 implement function *memset(...)*. What we do is change the name of *memset* to *xv6_memset* and use xv6_memset.
3. Always leave signature.  Xv6 uses *//* as its comments style, and we use */\*\*/* on every place we made modifications or add new contents.
4. Don't delete codes. We comment all codes that are not needed so far instead of deleting them. 
 
### What we've done
The booting process of xv6 can be briefly divided into several steps: bootstrap, information collection, memory allocation, traps & interrupts, locking,  scheduling and file system. Except bootstrap, other steps are listed clearly in *main.c* of the source code of xv6. Unfortunately, so far we only modified two of them, which are memory allocator and VFS.

Firstly, we modified the name of functions in **xv6_string.c**, it implements some common functions like *memset*, *memmove*, *strlen* etc. in traditional <string.c>, we add prefix 'xv6_' to each of them to distinguish, and use 'xv6_' functions in our project. Some header files can be copied directly, including **defs.h, dev.h param.h types.h x86.h**.

#### Memory Allocator 
Files involved: **kinit() - kalloc.c**.
##### kinit
We allocate a segment of memory from composite OS (1MB in our implementation). This segment should starts at a particular address (let's say 0xstart) and ends at 0xend, where 0xend - 0xstart = 1MB.

We consider this segment of memory as the VM's physical address space, and do a simple mapping to show the idea of the transformation between physical address and virtual address. We consider the virtual address of VM from 0x00800000 to 0x007000000, so the transformation formula is a simple math problem. As figure shows:

![](https://github.com/wfgydbu/paravirtualization/blob/master/trans/mmu.jpg)

**Note that we haven't implemented the mapping function, because it should be at lower level than what we are dealing with, probably in bootstrap step (which is a part of our future work). Instead, we provide two functions for address transformation to show this idea. These two functions named __mem_v2p and __mem_v2p in kalloc.c.**

##### freelist
Xv6 uses a sorted linked list, named *freelist* to manage its memory space. The *freelist* describes how much memory are currently free, each node in *freelist* has elements (addr, len, *next), representing a segment of free memory by its start address and length. See figure:

![](https://github.com/wfgydbu/paravirtualization/blob/master/trans/freelist.jpg)

The *freelist* is initialized as a single node, representing the whole memory space (a). Then in (b) a segment of memory is allocated, then the *freelist* is divided into two node. Each node represents a segment of free memeory. So as (c). If the first segment is freed in (c), the segments of memory represented by the first two node become continuous, they need to be combined. The implementation of *freelist* can be found in *kalloc()* and *kfree()* in *kalloc.c*.

#### Virtual File System (VFS)
Files involved: **ideinit() - disk.c ide.c**, **binit() - bio.c buh.c**, **iinit() - fs.c fs.h fsvar.h **, **fileinit() - file.c file.h stat.h**.

Xv6 describes a file system with 7 layers, see figure:

![](https://github.com/wfgydbu/paravirtualization/blob/master/trans/layer.jpg)

We will discuss it from bottom to top.

##### Disk
We use a char array to represent a virtual disk with size of 64MB. Then we hard code each part of this disk, see figure:

![](https://github.com/wfgydbu/paravirtualization/blob/master/trans/disk.jpg)

The virtual disk is constructed by sectors (or blocks), each sector is 512KB, in the figure, numbers at top represent how many sectors of each part, and numbers at bottom represent the start sector of each part.

Disk is divided into several parts: **boot** stores the booter content in the 1st sector; **superblock**: stores the information of how many inodes, how many data blocks etc. in this disk in sector 2; **log**; **inodes**; **bitmap** describes the usage of **data blocks**, the ratio of the size of bitmap to the size of data blocks is 1:8. Each part is constructed by one or more sectors.

We create a new file *disk.c* to implement this structure, and provide two interfaces *diskread()* and *diskwrite()* to support R/W operations with a particular sector.

##### Buffer Cache
Buffer cache is a cache for disk, with size of 10, using LRU as its replacement policy. Each cache line has the struct like this:
```
//buf.h
struct buf
{
    int flags;
    uint dev;
    uint sector;
    struct buf *prev; // LRU cache list
    struct buf *next;
    struct buf *qnext; // disk queue
    uchar data[512];
};
```
Three bits of the element *flags*  describe the status of this buf (BUSY, VALID and DIRTY), which further determines which operation (R or W) will be applied to this cache line; *dev* and *sector* describe the physical position of *data[512]*. One *buf* corresponds to one sector (or block).

Since we use a virtual disk, so the interface of buffer cache for reading and writing should be modified as well, that are *bread()* and *bwrite()*. Both of them call the function *iderw()*, and then use *flags* to determine what to do next, so we rewrite *iderw()* to make it work with our virtual disk.

##### Logging
Xv6 describes a simple journal logging mechanism in its document, however, logging isn't implemented in their codes. (their *inodes* part in the disk structure start at sector 2 leaves no space for logging.). The basic idea of logging is write a log to the log blocks on disk before each write operation to data blocks on disk.

We don't implement logging either, but we leave a space for logging in our disk structure, it starts at sector 2, and has length of 1024 sectors, we will implement it in the future.

##### Inode, Directory and Pathname
Xv6 implements these three parts in a single file *fs.c*, so we discuss them together. There are two concepts of *inode*, one is on-disk inodes, which is the physical data stored on the disk; another is in-memory inodes, which can be served as a "cache" to the on-disk inodes. The number of in-memory inodes has limited.

Each inode has an *inum* as identify. Its element *type* in *struct inode* represents the type of *data* in this inode, it could be a file, a directory or a special device. 

Then two functions related to directory, *dirlink()* and *dir	lookup()*, provide interface to construct the tree structure of logical file system starts with '/', whose inum in its inode is 1.

Functions related to Pathname are mainly used to resolve paths, like "/root/sys/init.conf".

Most of the functional codes can be used directly in these layers. But recall we changed the disk structure, so the formulas for calculating like which sector is a given inode on, or which bitmap block contains the information of a given data sector, should be modified carefully.

##### File descriptor
A file can either comes from a pipe, or a inode, we only discuss inode here. In this case, a file is basically an *inode* with its type as file, and some other attributes of a file. If you need to open a new file, the logic should probaly look like this:

![](https://github.com/wfgydbu/paravirtualization/blob/master/trans/accessfile.JPG)

Xv6 implements pipe, but we haven't, so we comments all codes related to pipe in *file.c*.

### An issue
We haven't implemented lock. But lock is required for most part of the file system to support multi-threads,  what we do for now is to comment all codes related to lock, and leave comments like "//LOCK_INIT*()", "//LOCK_TAKE()" and "//LOCK_RELEASE()", see figure:
```
// file.c
// Increment ref count for file f.
struct file *
filedup(struct file *f)
{
    //LOCK_TAKE();
    if (f->ref < 1)
	    PRINTC_ERR("filedup.\n");
    f->ref++;
    //LOCK_RELEASE();
    return f;
}
```
After we implement locks, we can simply define some macros at the header of the file, like:
```
lock lock_name;
#define LOCK_INIT() initlock(&lock_name, "lock_name");
#define LOCK_TAKE() acquire(&lock_name);
#define LOCK_RELEASE() release(&lock_name);
```

And remove the "//" of each comment, then lock should work, without going through all codes again.

### To be done
* Modify and implement lock and pipe.
* Re-read source codes of xv6.
* Modify and implement other parts of xv6,


## Email
Any questions or comments could be mailed to [huangyitian@gwu.edu](huangyitian@gwu.edu).