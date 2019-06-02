
#include <linux/linkage.h>
#include <linux/moduleloader.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include "shared.h"
asmlinkage extern long (*sysptr)(void *arg);

asmlinkage long cpenc(void *arg)
{    struct file *fd_in;
    struct file *fd_out;
        //printk("cpenc received arg %s\n",arg->arg1);
	//printk("cpenc received arg %s \n",arg->arg2);
	//char buf[4];
	struct myargs *buf;
		//kmalloc(sizeof(struct card), GFP_KERNEL); 
	buf=kmalloc(sizeof(struct myargs),GFP_KERNEL);	
//	printk("Size of buf: %lu\n", sizeof(*buf));
	copy_from_user(buf,arg,sizeof(struct myargs));
	//kmalloc(sizeof(my_struct),GFP_KERNEL); 
	printk("Input file %s\n",buf->arg1);
	printk("Output file %s\n",buf->arg2);
        fd_in = filp_open(buf->arg1, O_RDONLY, 0);
	if (IS_ERR(fd_in)) {
    	printk("%ld\n", PTR_ERR(fd_in));
	}
	//permissions might be affected with umask 022 for root
	fd_out = filp_open(buf->arg2, O_WRONLY|O_CREAT, 0555);
        if (IS_ERR(fd_out)) {
        printk("%ld\n", PTR_ERR(fd_out));
        }
	//buf->arg2 Earlier I'm using buffer as an object and the buffer here is a pointer
	/* dummy syscall: returns 0 for non null, -EINVAL for NULL */
	//printk("cpenc received arg pointer %p\n", arg);
	kfree(buf);
	/*file closing for  */
filp_close(fd_in,0);
filp_close(fd_out,0);

	if (arg == NULL)
		return -EINVAL;
	else
		return 0;
}

static int __init init_sys_cpenc(void)
{
	printk("installed new sys_cpenc module\n");
	if (sysptr == NULL)
		sysptr = cpenc;
	return 0;
}
static void  __exit exit_sys_cpenc(void)
{
	if (sysptr != NULL)
		sysptr = NULL;
	printk("removed sys_cpenc module\n");
}
module_init(init_sys_cpenc);
module_exit(exit_sys_cpenc);
MODULE_LICENSE("GPL");
