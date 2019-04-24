#include <linux/linkage.h>
#include <linux/moduleloader.h>
#include <linux/uaccess.h>
#include "shared.h"
asmlinkage extern long (*sysptr)(void *arg);

asmlinkage long cpenc(void *arg)
{
        //printk("cpenc received arg %s\n",arg->arg1);
	//printk("cpenc received arg %s \n",arg->arg2);
	//char buf[4];
	struct myargs buf;
	printk("Size of buf: %lu\n", sizeof(buf));

	copy_from_user(&buf,arg,sizeof(buf));
	printk("Input file1 %s\n",buf.arg2);
        
	/* dummy syscall: returns 0 for non null, -EINVAL for NULL */
	printk("cpenc received arg %p\n", arg);
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
