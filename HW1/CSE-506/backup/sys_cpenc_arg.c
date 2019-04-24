#include <linux/linkage.h>
#include <linux/moduleloader.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include "shared.h"
#include <crypto/internal/skcipher.h>
/*Keep them in shared.h like earlier*/

/*#define data_size 4096*/
asmlinkage extern long (*sysptr)(void *arg);

long int file_read(struct file *file,  char *data, unsigned long size, unsigned long long offset);
long int file_write(struct file *file,  char *data, unsigned long size, unsigned long long offset);

/* Initialize and trigger cipher operation */

 /* This code is composed of initializing the file pointers and checking the arguments properly.
 *  *
 *   * Checking for the errors in case of bad address allocations
 *    *
 *     *
 *      *
 *       *
 *        *
 *         *Closing the files and freeing the kmalloc allocating pointers 
 *          */

 asmlinkage long cpenc(void *arg)
 {
	int data_size=4096;
	bool read_flag = true;
	int retval=0;
//        char *key=NULL; 
	struct filename *file_in, *file_out;
myargs *buf;
         buf=kmalloc(sizeof(myargs),GFP_KERNEL);
         if (!buf){
                 printk(KERN_ERR "Unable to allocate memory.");
                 return -EFAULT;
         }
         if (copy_from_user(buf, arg, sizeof(myargs)))
                 return -EFAULT;
//      key=kmalloc(sizeof(*(buf->arg3)),GFP_KERNEL);
//	key=buf->arg3;
	char *key=buf->arg3;
	printk("key %s", key);
	printk("input %s\n", buf->arg1);
	printk("output %s\n", buf->arg2);
	printk("password %s\n", buf->arg3);
	printk("length %d\n", buf->length);
	file_in = getname(((myargs *)arg)->arg1);
	file_out=getname(((myargs *)arg)->arg2);
	/*Cleaning up the code*/
  out:
    if(buf)
	kfree(buf);    
//    if(key)
//	kfree(key);
    return retval;
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
