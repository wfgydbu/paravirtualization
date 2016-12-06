#include "xv6.h"

struct cos_compinfo booter_info;
thdcap_t termthd; 		/* switch to this to shutdown */
thdcap_t initthd; 	
unsigned long tls_test[TEST_NTHDS];

extern void kinit(void);
extern void ideinit(void);
extern void binit(void);
extern void fileinit(void);
extern void iinit(void);

static void
cos_llprint(char *s, int len)
{ call_cap(PRINT_CAP_TEMP, (int)s, len, 0, 0); }

int
prints(char *s)
{
	int len = strlen(s);

	cos_llprint(s, len);

	return len;
}

int __attribute__((format(printf,1,2)))
printc(char *fmt, ...)
{
	  char s[128];
	  va_list arg_ptr;
	  int ret, len = 128;

	  va_start(arg_ptr, fmt);
	  ret = vsnprintf(s, len, fmt, arg_ptr);
	  va_end(arg_ptr);
	  cos_llprint(s, ret);

	  return ret;
}

/* For Div-by-zero test */
int num = 1, den = 0;

void
term_fn(void *d)
{ SPIN(); }

void
xv6_main(void *d){
	//PRINTC("main.c\n");
	//mpinit(); // collect info about this machine
	//lapicinit(mpbcpu());
	//ksegment();
	//picinit();       // interrupt controller
	//ioapicinit();    // another interrupt controller
	//consoleinit();   // I/O devices & their interrupts
	//uartinit();      // serial port
	//printfc("\ncpu%d: starting xv6\n\n", cpu());

	kinit();         // physical memory allocator
	
	//pinit();         // process table
	//tvinit();        // trap vectors
	ideinit();      	 // disk /* ethan: virtual */
	binit();         // buffer cache
	iinit();         // inode cache
	fileinit();      // file table
	//if(!ismp)
	//	timerinit();   // uniprocessor timer
	//userinit();      // first user process
	//bootothers();    // start other processors

	// Finish setting up this processor in mpmain.
	//mpmain();

	cos_thd_switch(termthd);
}



void
cos_init(void *d)
{
	int cycs;

	cos_meminfo_init(&booter_info.mi, BOOT_MEM_KM_BASE, COS_MEM_KERN_PA_SZ, BOOT_CAPTBL_SELF_UNTYPED_PT);
	cos_compinfo_init(&booter_info, BOOT_CAPTBL_SELF_PT, BOOT_CAPTBL_SELF_CT, BOOT_CAPTBL_SELF_COMP,
			  (vaddr_t)cos_get_heap_ptr(), BOOT_CAPTBL_FREE, &booter_info);

	termthd = cos_thd_alloc(&booter_info, booter_info.comp_cap, term_fn, NULL);
	assert(termthd);

	initthd = cos_thd_alloc(&booter_info, booter_info.comp_cap, xv6_main, NULL);
	assert(initthd);

	while (!(cycs = cos_hw_cycles_per_usec(BOOT_CAPTBL_SELF_INITHW_BASE))) ;
	printc("\t%d cycles per microsecond\n", cycs);

	PRINTC("Xv6 Booter started.\n");
	
	/*start main.c */
	cos_thd_switch(initthd);
	//test_run_vk();
	PRINTC("Xv6 Booter done.\n");

	cos_thd_switch(termthd);
	
	return;
}
