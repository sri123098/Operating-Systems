#include <linux/linkage.h>
#include <linux/moduleloader.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <crypto/internal/skcipher.h>
typedef struct {
	char *arg1;
	char *arg2;
}myargs;
//Keep them in shared.h like earlier

//#define data_size 4096
asmlinkage extern long (*sysptr)(void *arg);

long int file_read(struct file *file,  char *data, unsigned long size, unsigned long long offset);
long int file_write(struct file *file,  char *data, unsigned long size, unsigned long long offset);

/* Initialize and trigger cipher operation */
static int test_skcipher(void)
{
    char *scratchpad="abcd";
 /* Input data will be random */
    printk("printing the scratchpad %s", scratchpad);
	return 0;
}


/*
 * This code is composed of initializing the file pointers and checking the arguments properly.
 *
 * Checking for the errors in case of bad address allocations
 *
 *
 *
 *
 *
 *Closing the files and freeing the kmalloc allocating pointers 
 */

 asmlinkage long cpenc(void *arg)
 {
	//struct skcipher_def *sk;
	//int enc=1;
	struct file *fd_in, *fd_out;
	struct filename *file_in, *file_out;
	file_in = getname(((myargs *)arg)->arg1);
	file_out=getname(((myargs *)arg)->arg2);
	ssize_t bytes_read, bytes_write;
        int data_size=4096;
	char *data;/*data_size is the chunk with which data is read*/
	bool read_flag = true;
	int retval=0;
	unsigned long inode_input, inode_output;
        /*Verifying whether file descriptors are proper or not*/
	printk("Starting the skcipher(void)");
	int test=test_skcipher();
	printk("Output of test_skcipher%d",test);
	printk("Ending the skcipher(void)"); 
        
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
        //printk("printing the inode number of input %d", fd_in->inode);
 
	fd_out = filp_open(file_out->uptr, O_WRONLY | O_CREAT, 0777);
         
	if (!fd_out || IS_ERR(fd_out)) {
            printk("%ld\n", PTR_ERR(fd_out));
	    retval= -ENOENT;
	    goto out;
         }
	//printk("printing the inode number of output %d", fd_out->inode);
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
	 * Reading the file in chunks
	 * Writing the file in chunks
	 * Chunk file size or page size */
	data=kmalloc(data_size,GFP_KERNEL);
         /*Checking for the pointer and returns bad address if it is not proper*/
         if (!data){
	         printk(KERN_ERR "Unable to allocate memory.");
	         retval=-EFAULT;
		goto out;
         }
	mm_segment_t old_fs;
	old_fs= get_fs();
	set_fs(KERNEL_DS);
	while(read_flag)
	{
		bytes_read=vfs_read(fd_in, data, data_size, &fd_in->f_pos);
		if (bytes_read < 0)
		{
			retval = -EINTR;
		  	goto out;
		}
		if (bytes_read < data_size)
		{
			read_flag = false; 
		}
		bytes_write=vfs_write(fd_out, data, bytes_read,&fd_out->f_pos);
	}
	set_fs(old_fs);
	printk("Output of file_read---> %ld  and fd_in->f_pos-----> %llu \n",bytes_write,fd_out->f_pos);
 
	/*Cleaning up the code*/
  out:
	if (data)
		kfree(data); 
         //kfree(buf);
         if(fd_in)
         	filp_close(fd_in,0);
         if(fd_out)
		filp_close(fd_out,0);
         return retval;
	 if (arg == NULL)
         	return -EINVAL;
         else
         	return 0;
 }
/*
        while(read_flag)
	{
		bytes_read=file_read(fd_in, data, data_size, fd_in->f_pos);
		if (bytes_read < data_size)
		{
			read_flag = false; 
		}
		bytes_write=file_write(fd_out, data, bytes_read, fd_out->f_pos);
	}
	*/

//permissions thing is pending
//Encryption /Decryption 
//Lot of validations are pending

//Locks are pending
////kmalloc(sizeof(struct card), GFP_KERNEL); 
////kmalloc(sizeof(my_struct),GFP_KERNEL); 
//         //return -EINVAL;
//          //Add an extra error ENOENT
//                   //return -EINVAL;
//                    //Add an extra error ENOENT
//                             //permissions might be affected with umask 022 for root
//                               //buf->arg2 Earlier I'm using buffer as an object and the buffer here is a pointer
//                                        /* dummy syscall: returns 0 for non null, -EINVAL for NULL */
//                                                 //printk("cpenc received arg pointer %p\n", arg);
//
// How to read end of the file?

/*
 * *The following things are needed to verify the below coded modules as per 4.20.6 elixir
 * *ssize_t vfs_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
 * *check the data types of size_t and ssize_t The mapping is done to 
 * *typedef unsigned long __kernel_size_t;
 * *typedef long          __kernel_ssize_t;
 * *typedef long long     __kernel_loff_t;
 * *long int -%ld In old kernel, unsigned char is used
 * *#define KERNEL_DS	((mm_segment_t) { 0UL })
 * *#define USER_DS		((mm_segment_t) { -0x40000000000UL })
 * *#define get_ds()  (KERNEL_DS)------Later we can change to get_ds()
 * *ssize_t vfs_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
 * */


/*
 * * file_read - Reads from a file and returns long integer
 * * @file: file pointer in which you have opened in system callwhose metadata to set
 * * @data: data being read is kept in data pointer
 * * @size: size of the data being read from the offset
 * * @offset: offset similar to loff_t
 * */

long int file_read(struct file *file,  char *data, unsigned long size, unsigned long long offset) 
{
    mm_segment_t oldfs;
    long int ret;
    oldfs = get_fs();
    set_fs(KERNEL_DS);
    ret = vfs_read(file, data, size, &offset);
    set_fs(oldfs);
    return ret;
} 


/*
 * * file_write - Reads from a file and returns long integer
 * * @file: file pointer in which you have opened in system callwhose metadata to set
 * * @data: data being read is kept in data pointer
 * * @size: size of the data being read from the offset
 * * @offset: offset similar to loff_t
 * */
long int file_write(struct file *file,  char *data, unsigned long size, unsigned long long offset)
{
    mm_segment_t oldfs;
    long int ret;
    oldfs = get_fs();
    set_fs(KERNEL_DS);
    ret = vfs_write(file, data, size, &offset);
    set_fs(oldfs);
    return ret;
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
