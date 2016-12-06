#include "types.h"
#include "defs.h"
#include "param.h"
#include "file.h"
//#include "spinlock.h"
#include "dev.h"
#include "xv6.h"

struct devsw devsw[NDEV];
struct
{
    //struct spinlock lock;
    struct file file[NFILE];
} ftable;

void __test_file();

void fileinit(void)
{
    PRINTC("(fileinit) - file descriptor.\n");
    //LOCK_INIT();
    __test_file();
}

// Allocate a file structure.
struct file *
filealloc(void)
{
    struct file *f;

    //LOCK_TAKE();
    for (f = ftable.file; f < ftable.file + NFILE; f++)
    {
        if (f->ref == 0)
        {
            f->ref = 1;
            //LOCK_RELEASE();
            return f;
        }
    }
    //LOCK_RELEASE();
    return 0;
}

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

// Close file f.  (Decrement ref count, close when reaches 0.)
void fileclose(struct file *f)
{
      struct file ff;

      //LOCK_TAKE();
      if(f->ref < 1)
        PRINTC_ERR("fileclose");
      if(--f->ref > 0){
        //LOCK_RELEASE();
        return;
      }
      ff = *f;
      f->ref = 0;
      f->type = FD_NONE;
      //LOCK_RELEASE();

      if(ff.type == FD_PIPE){
        /* ethan: pipe not suported now */          
        //pipeclose(ff.pipe, ff.writable);
        PRINTC_ERR("fileclose: pipe\n"); 
        return;
      }        
      else if(ff.type == FD_INODE)
        iput(ff.ip);
}

// Get metadata about file f.
int filestat(struct file *f, struct xv6_stat *st)
{
    if (f->type == FD_INODE)
    {
        ilock(f->ip);
        stati(f->ip, st);
        iunlock(f->ip);
	    return 0;
    }
    return -1;
}

// Read from file f.  Addr is kernel address.
int fileread(struct file *f, char *addr, int n)
{
    return 0;
      int r;

      if(f->readable == 0)
        return -1;
      if(f->type == FD_PIPE){
        /* ethan: pipe not suported now */          
        //return piperead(f->pipe, addr, n);
        PRINTC_ERR("fileread: pipe\n"); 
        return 0;
      }  

        
      if(f->type == FD_INODE){
        ilock(f->ip);
        if((r = readi(f->ip, addr, f->off, n)) > 0)
          f->off += r;
        iunlock(f->ip);
        return r;
      }
      PRINTC_ERR("fileread");
}

// Write to file f.  Addr is kernel address.
int filewrite(struct file *f, char *addr, int n)
{
    return 0;
      int r;

      if(f->writable == 0)
        return -1;
      if(f->type == FD_PIPE){
        /* ethan: pipe not suported now */          
        //return pipewrite(f->pipe, addr, n);
        PRINTC_ERR("filewrite: pipe\n"); 
        return 0;
      }        

      if(f->type == FD_INODE){
        ilock(f->ip);
        if((r = writei(f->ip, addr, f->off, n)) > 0)
          f->off += r;
        iunlock(f->ip);
        return r;
      }
      PRINTC_ERR("filewrite.\n");
}

/* ethan:
 * test file functions.
 */
 void 
 __test_file()
 {
     PRINTC("\tTest:\n");
     
     struct file *f = filealloc();

     PRINTC("\t\tAllocate a file, with refs: %d\n",f->ref);

     f = filedup(f);
     PRINTC("\t\tTest increment refs: %d\n",f->ref);

     /* reset */
     f->ref = 1;

     char s[16],d[16]; 
     filewrite(f,s,16);
     fileread(f,d,16);
     PRINTC("\t\tTest W/R with files.\n");

     fileclose(f);



     PRINTC("\tTest Done.\n");
 }