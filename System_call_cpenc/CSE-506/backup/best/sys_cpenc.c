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
	char *data=NULL;/*data_size is the chunk with which data is read*/
	bool read_flag = true;
	int retval=0;
	unsigned long inode_input, inode_output;
	struct file *fd_in, *fd_out;
	struct filename *file_in, *file_out;
	ssize_t bytes_read, bytes_write;
//    int ret = -EFAULT;
    struct crypto_skcipher *skcipher = NULL;
    struct skcipher_request *req = NULL;
    char ivdata[16] = "itshouldnotcorru";
    unsigned char *key=NULL;
    struct scatterlist sg;
  	 mm_segment_t old_fs;
         myargs *buf;
         buf=kmalloc(sizeof(myargs),GFP_KERNEL);
         /*Checking for the pointer and returns bad address if it is not proper*/
         if (!buf){
                 printk(KERN_ERR "Unable to allocate memory.");
                 return -EFAULT;
         }
         if (copy_from_user(buf, arg, sizeof(myargs)))
                 return -EFAULT;
	key=buf->arg3;
	printk("key %s", key);
	printk("input %s", buf->arg1);
	printk("input %s", buf->arg2);

	file_in = getname(((myargs *)arg)->arg1);
	file_out=getname(((myargs *)arg)->arg2);
	skcipher = crypto_alloc_skcipher("ctr(aes)", 0, 0);
    if (IS_ERR(skcipher)) {
        pr_info("could not allocate skcipher handle\n");
        return PTR_ERR(skcipher);
    }
    req = skcipher_request_alloc(skcipher, GFP_KERNEL);
    if (!req) {
        pr_info("could not allocate skcipher request\n");
        retval = -ENOMEM;
        goto out;
    }
    if (crypto_skcipher_setkey(skcipher, key, 16)) {
        pr_info("key could not be set\n");
        retval = -EAGAIN;
        goto out;
    }
        /*Verifying whether file descriptors are proper or not*/
	/*verifying the struct filename pointers */
	if (!file_in || IS_ERR(file_in)){
		printk("%ld\n",PTR_ERR(file_in));
		retval=-EFAULT;
		goto out;
	}
	if (!file_out || IS_ERR(file_out)){
		printk("%ld\n",PTR_ERR(file_out));
		retval=-EFAULT;
		goto out;
	}
	fd_in = filp_open(file_in->uptr, O_RDONLY, 0);
         if (!fd_in || IS_ERR(fd_in)) {
	         printk("%ld\n", PTR_ERR(fd_in));
	         retval=-ENOENT;
		 goto out;
         }
        /*printk("printing the inode number of input %d", fd_in->inode);*/
 
	fd_out = filp_open(file_out->uptr, O_WRONLY | O_CREAT, 0777);
         
	if (!fd_out || IS_ERR(fd_out)) {
            printk("%ld\n", PTR_ERR(fd_out));
	    retval= -ENOENT;
	    goto out;
         }
	/*printk("printing the inode number of output %d", fd_out->inode);*/
         printk("Input file %s\n Output file %s\n ",file_in->uptr, file_out->uptr);
         fd_in->f_pos=0;
	 fd_out->f_pos=0;
	inode_input  = fd_in->f_path.dentry->d_inode->i_ino ;
	inode_output = fd_out->f_path.dentry->d_inode->i_ino ; 
	printk("Inode number of input file %lu\n", fd_in->f_path.dentry->d_inode->i_ino);
	printk("Inode number of output file %lu\n", fd_out->f_path.dentry->d_inode->i_ino);
	if (inode_input == inode_output)
	{
	 retval=-EINVAL;
         goto out;
	}
	/*Real Functionality
 * 	 * Reading the file in chunks
 * 	 	 * Writing the file in chunks
 * 	 	 	 * Chunk file size or page size */
	 data=kmalloc(data_size,GFP_KERNEL);
         /*Checking for the pointer and returns bad address if it is not proper*/
         if (!data){
	         printk(KERN_ERR "Unable to allocate memory.");
	         retval=-EFAULT;
		goto out;
         }
	old_fs= get_fs();
	set_fs(KERNEL_DS);
	

	


while(read_flag)
	{
		bytes_read=vfs_read(fd_in, data, data_size, &fd_in->f_pos);
		if (bytes_read < 0)
		{
			retval = -EINTR;
           		set_fs(old_fs);
		  	goto out;
		}
        printk("data %s and bytes_read %ld", data, bytes_read);	
        sg_init_one(&sg, data, bytes_read);
        skcipher_request_set_crypt(req, &sg, &sg, bytes_read, ivdata);
        retval=crypto_skcipher_encrypt(req);
        if(retval)
            {
	    set_fs(old_fs);
            goto out;
		}
        printk("data %s and bytes_read %ld", data, bytes_read);	
	printk("After encryption \ndata %s and bytes_read %ld", data, bytes_read);
	
	
	if (bytes_read < data_size)
		{
			read_flag = false; 
		}
		bytes_write=vfs_write(fd_out, data, bytes_read,&fd_out->f_pos);
	}/*While loop*/
	set_fs(old_fs);
	printk("Output of file_read---> %ld  and fd_in->f_pos-----> %llu \n",bytes_write,fd_out->f_pos);
	/*Cleaning up the code*/
  out:
    if (skcipher)
        crypto_free_skcipher(skcipher);
    if (req)
        skcipher_request_free(req);
    if (data)
	kfree(data); 
    if(fd_in)
        filp_close(fd_in,0);
    if(fd_out)
	filp_close(fd_out,0);
    if(buf)
	kfree(buf);    
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
