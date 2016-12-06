// Physical memory allocator, intended to allocate
// memory for user processes. Allocates in 4096-byte "pages".
// Free list is kept sorted and combines adjacent pages into
// long runs, to make it easier to allocate big segments.
// One reason the page size is 4k is that the x86 segment size
// granularity is 4k.
#include <cos_synchronization.h>

#include "types.h"
#include "defs.h"
#include "param.h"
//#include "spinlock.h"
#include "xv6.h"

struct run
{
    struct run *next;
    int len; // bytes
};

struct
{
    //struct spinlock lock;
    struct run *freelist;
} kmem;

char *start, *end;

cos_lock_t xv6_mem_lock;
#define LOCK_TAKE()                                  \
    do                                               \
    {                                                \
	if (unlikely(lock_take(&xv6_mem_lock) != 0)) \
	    BUG();                                   \
    } while (0)
#define LOCK_RELEASE()                                  \
    do                                                  \
    {                                                   \
	if (unlikely(lock_release(&xv6_mem_lock) != 0)) \
	    BUG()                                       \
    } while (0)
#define LOCK_INIT() lock_static_init(&xv6_mem_lock);

static void __kalloc_test(void);
static uint __mem_v2p(char *s); /* virtual addr to physical addr */
static uint __mem_p2v(char *s);

// Initialize free list of physical pages.
// This code cheats by just considering one megabyte of
// pages after _end.  Real systems would determine the
// amount of memory available in the system and use it all.
#define XV6_PAGE_SIZE 4096
#define NPAGES (1024 / 4) /* 256 pages, each 4KB */
#define XV6_MEM_BASE 0x00800000

void kinit(void)
{
    PRINTC("(kinit) - memory allocator\n");

    /* ethan: allocate 1MB mem for the xv6 user processes */
    char *s, *t, *prev, *t1;

    int i;

    s = cos_page_bump_alloc(&booter_info);
    assert(s);

    prev = s;
    for (i = 0; i < NPAGES; i++)
    {
	t = cos_page_bump_alloc(&booter_info);
	//PRINTC("[%d]0x%x\n", i, (unsigned int)t);
	assert(t && t == prev + 4096);

	prev = t;
    }
    xv6_memset(s, 0, NPAGES * 4096);
    PRINTC("\t[COS]Allocated %d pages = %d MB memory\n", NPAGES, NPAGES * 4096 / 1024 / 1024);
    PRINTC("\t[COS]First address: 0x%x, Last address: 0x%x\n", (uint)s, (uint)t);

    //LOCK_INIT();
    start = s;
    end = t;

    kfree(start, NPAGES * 4096);

    /* test for memory allocator */

    __kalloc_test();
}

// Free the len bytes of memory pointed at by v,
// which normally should have been returned by a
// call to kalloc(len).  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(char *v, int len)
{
    struct run *r, *rend, **rp, *p, *pend;

    if (len <= 0 || len % PAGE)
	PRINTC_ERR("kfree: invalid value. (len <= 0) or (len != n*PAGE).\n");

    // Fill with junk to catch dangling refs.
    xv6_memset(v, 1, len);

    // lock_take(&xv6_mem_lock);
    p = (struct run *)v;
    pend = (struct run *)(v + len);
    for (rp = &kmem.freelist; (r = *rp) != 0 && r <= pend; rp = &r->next)
    {
	rend = (struct run *)((char *)r + r->len);
	if (r <= p && p < rend)
	    PRINTC_ERR("freeing free page");
	if (pend == r)
	{ // p next to r: replace r with p
	    p->len = len + r->len;
	    p->next = r->next;
	    *rp = p;
	    goto out;
	}
	if (rend == p)
	{ // r next to p: replace p with r
	    r->len += len;
	    if (r->next && r->next == pend)
	    { // r now next to r->next?
		r->len += r->next->len;
		r->next = r->next->next;
	    }
	    goto out;
	}
    }
    // Insert p before r in list.
    p->len = len;
    p->next = r;
    *rp = p;

out:
    return;
    // lock_release(&xv6_mem_lock);
}

// Allocate n bytes of physical memory.
// Returns a kernel-segment pointer.
// Returns 0 if the memory cannot be allocated.
char *
kalloc(int n)
{
    char *p;
    struct run *r, **rp;

    if (n % PAGE || n <= 0)
	PRINTC_ERR("kalloc");

    //LOCK_TAKE();
    for (rp = &kmem.freelist; (r = *rp) != 0; rp = &r->next)
    {
	if (r->len == n)
	{
	    *rp = r->next;
	    // LOCK_RELEASE();
	    return (char *)r;
	}
	if (r->len > n)
	{
	    r->len -= n;
	    p = (char *)r + r->len;
	    //LOCK_RELEASE();
	    return p;
	}
    }
    //LOCK_RELEASE();

    PRINTC_ERR("kalloc: out of memory\n");
    return 0;
}

/* ethan:
 * run some tests for kalloc(..) and kfree(..)
 */
#define PAGES_3 (3 * XV6_PAGE_SIZE)
#define PAGES_4 (4 * XV6_PAGE_SIZE)
#define PAGES_5 (5 * XV6_PAGE_SIZE)

static void
__kalloc_test(void)
{
    PRINTC("\tTest:\n");

    char *s, *t, *j;

    s = kalloc(PAGES_3);
    PRINTC("\t\t0x%x => 0x%08x\n", (uint)s, __mem_v2p(s));

    t = kalloc(PAGES_5);
    PRINTC("\t\t0x%x => 0x%08x\n", (uint)t, __mem_v2p(t));

    kfree(t, PAGES_5);
    t = kalloc(PAGES_5);
    PRINTC("\t\t0x%x => 0x%08x\n", (uint)t, __mem_v2p(t));

    kfree(s, PAGES_3);
    j = kalloc(PAGES_4);
    PRINTC("\t\t0x%x => 0x%08x\n", (uint)j, __mem_v2p(j));

    s = kalloc(PAGES_3);
    PRINTC("\t\t0x%x => 0x%08x\n", (uint)s, __mem_v2p(s));

    PRINTC("\tTest Done.\n");
}

/* ethan: 
 * convert virtual memory to "physical memory", and vice versa
 * simple math
 */
static uint
__mem_v2p(char *s)
{
    return (uint)s - (uint)end + XV6_MEM_BASE;
}

static uint
__mem_p2v(char *s)
{
    return (uint)s + (uint)end - XV6_MEM_BASE;
}