#include <linux/linkage.h>
#include <linux/moduleloader.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include "shared.h"
#include <crypto/hash.h>
#include <linux/crypto.h>
#include <crypto/internal/skcipher.h>
#define BUF_SIZE 4096
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
 *        *  So you can break your code into several smaller function (a good thing for code clarity) and still not hurt
 *        stack size.
 *         *Closing the files and freeing the kmalloc allocating pointers 
 *          */
int MD5_hash(char* passkey, int keylen, char* key){
	int retval = 0;
	struct shash_desc* desc = (struct shash_desc*)kmalloc(sizeof(struct shash_desc), GFP_KERNEL);
	struct crypto_shash* cryp_shash = (struct crypto_shash*)crypto_alloc_shash("md5", 0, CRYPTO_ALG_ASYNC);
	if (!cryp_shash || IS_ERR(cryp_shash)){
            retval = (int)PTR_ERR(cryp_shash);
            return retval;
    }
	desc->tfm = cryp_shash;
	retval = crypto_shash_digest(desc, passkey, keylen, key);
	kfree(desc);
	crypto_free_shash(cryp_shash);
	return retval;
}
/*
 * returns 0 if the message digest creation was successful 
 * < 0 if an error occured
 **/ 
 
 asmlinkage long cpenc(void *arg)
 {
	char *data=NULL;/*BUF_SIZE is the chunk with which data is read*/
	bool read_flag = true;
	int retval=0;
	unsigned long inode_input, inode_output;
	struct file *fd_in, *fd_out;
	struct filename *file_in, *file_out;
	ssize_t bytes_read, bytes_write;
    struct crypto_skcipher *skcipher = NULL;
    struct skcipher_request *req = NULL;
    char ivdata[16] = "Nomemoryleakallo";
    unsigned char *key=NULL;
    struct scatterlist sg;
    int flag,i,delval=0;	
    long long *size_of_input;
	int hash_check;
    char *hash_key;
    struct kstat *stat;
    mm_segment_t old_fs;
    myargs *buf;
    size_of_input = kmalloc(sizeof(long long),GFP_KERNEL);
    stat=kmalloc(sizeof(struct kstat),GFP_KERNEL);
    hash_key=kmalloc(sizeof(char)*16,GFP_KERNEL);
    key=kmalloc(sizeof(char)*16,GFP_KERNEL);
    buf=kmalloc(sizeof(myargs),GFP_KERNEL);
         /*Checking for the pointer and returns bad address if it is not proper*/
    if (!buf){
        printk(KERN_ERR "Unable to allocate memory.");
        return -EFAULT;
    }
    if (copy_from_user(buf, arg, sizeof(myargs)))
        return -EFAULT;
	key=buf->arg3;
//    hash_check=MD5_hash(key, 16, hash_key);
//	printk("Kernel hashed key\n");
//    for(i=0;i<16;i++)
//        printk(KERN_CONT "%02x",((const u8*)hash_key)[i]);
    flag=buf->flag;
	printk("\nUSER key\n");
    for(i=0;i<16;i++)
        printk(KERN_CONT "%02x", key[i]);
    printk("flag %d",flag);
//	printk("key %s", key);
	printk("input %s\n", buf->arg1);
	printk("Output %s\n", buf->arg2);
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
    /*hash key to be changed*/
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
	 data=kmalloc(BUF_SIZE,GFP_KERNEL);
         /*Checking for the pointer and returns bad address if it is not proper*/
     if (!data){
	    printk(KERN_ERR "Unable to allocate memory.");
	    retval=-EFAULT;
		goto out;
    }
//old_fs= get_fs();
//set_fs(KERNEL_DS);
retval=vfs_stat(((myargs *)arg)->arg1,stat);
//set_fs(old_fs);
if (retval!=0){
    printk("vfs_stat issue\n");
    goto out;
}

goto out;
size_of_input=&(stat->size);
printk("Size of the file lli format %lld",stat->size);
    
//You can use stat->size and f_pos    
    
    
    printk("Input file %s\n Output file %s\n ",file_in->uptr, file_out->uptr);
    fd_in->f_pos=0;
	fd_out->f_pos=0;
	inode_input  = fd_in->f_path.dentry->d_inode->i_ino ;
	inode_output = fd_out->f_path.dentry->d_inode->i_ino ; 
	printk("Inode number of input file %lu\n", fd_in->f_path.dentry->d_inode->i_ino);
	printk("Inode number of output file %lu\n", fd_out->f_path.dentry->d_inode->i_ino);
	if (inode_input == inode_output){
	     retval=-EINVAL;
         goto out;
	}
    
    if(true)
    {goto out;
    }
    
    
    if(flag & 0x1){
        old_fs= get_fs();
	    set_fs(KERNEL_DS);
        bytes_write=vfs_write(fd_out, key, 16,&fd_out->f_pos);
        if (bytes_write < 0){
            retval = -EINTR;
            set_fs(old_fs);
            goto out;
        }
//Need to enhance the existing code with vfs stat
        while(read_flag){
		    bytes_read=vfs_read(fd_in, data, BUF_SIZE, &fd_in->f_pos);
		    if (bytes_read < 0){
			    retval = -EINTR;
           		set_fs(old_fs);
		  	    goto out;
                //Need to write the code for unlink
		    }
	        //printk("data %s and bytes_read %ld", data, bytes_read);	
	        sg_init_one(&sg, data, bytes_read);
	        skcipher_request_set_crypt(req, &sg, &sg, bytes_read, ivdata);
	        retval=crypto_skcipher_encrypt(req);
	        if(retval){
		        set_fs(old_fs);
                retval = -EINTR;
	            goto out;
		    }
	       // printk("data %s and bytes_read %ld", data, bytes_read);	
		   // printk("After encryption \ndata %s and bytes_read %ld", data, bytes_read);
		    if (bytes_read < BUF_SIZE){
			    read_flag = false; 
		    }
		    bytes_write=vfs_write(fd_out, data, bytes_read,&fd_out->f_pos);
            if (bytes_write != bytes_read){
                retval = -EINTR;
                set_fs(old_fs);
                goto out;
                }//Need to write code for unlink
	    }/*While loop*/
	    set_fs(old_fs);
        
        if(fd_out->f_pos !=(*size_of_input+16)){
            retval = -EINTR;
            goto out;
        }
	    printk("Output of file_read---> %ld  and fd_in->f_pos-----> %llu \n",bytes_write,fd_out->f_pos);
	}
    /*Code for Decryption*/
    else if(flag & 0x2){
        old_fs= get_fs();
	    set_fs(KERNEL_DS);
        bytes_read=vfs_read(fd_in, data, 16,&fd_in->f_pos);
        if (bytes_read < 0){
            retval = -EINTR;
            set_fs(old_fs);
            goto out;
        }
        if (memcmp(data,key,16)!=0){
            retval=-EACCES;
            set_fs(old_fs);
            goto out;
        }
	    while(read_flag){
		    bytes_read=vfs_read(fd_in, data, BUF_SIZE, &fd_in->f_pos);
		    if (bytes_read < 0){
			    retval = -EINTR;
           		set_fs(old_fs);
		  	    goto out;
		    }
	        //printk("data %s and bytes_read %ld", data, bytes_read);	
	        sg_init_one(&sg, data, bytes_read);
	        skcipher_request_set_crypt(req, &sg, &sg, bytes_read, ivdata);
	        retval=crypto_skcipher_decrypt(req);
	        if(retval){
		        set_fs(old_fs);
                printk("cipher problem in decrypting");
                retval=-EINTR;
	            goto out;
		    }
	      //  printk("data %s and bytes_read %ld", data, bytes_read);	
		   // printk("After Decryption \n data %s and bytes_read %ld", data, bytes_read);
		    if (bytes_read < BUF_SIZE){
			    read_flag = false; 
		    }
		    bytes_write=vfs_write(fd_out, data, bytes_read,&fd_out->f_pos);
	        if (bytes_read !=bytes_write){
                set_fs(old_fs);
                retval = -EINTR;
                goto out;
            }
        }/*While loop*/
	    set_fs(old_fs);
        if(fd_out->f_pos !=(*size_of_input-16)){
            retval = -EINTR;
            goto out;
        }
        printk("Output of file_read---> %ld  and fd_in->f_pos-----> %llu \n",bytes_write,fd_out->f_pos);
	}
    /*Code for Copy*/
    else if(flag & 0x4){
        old_fs= get_fs();
	    set_fs(KERNEL_DS);
	    while(read_flag){
		    bytes_read=vfs_read(fd_in, data, BUF_SIZE, &fd_in->f_pos);
		    if (bytes_read < 0){
			    retval = -EINTR;
           		set_fs(old_fs);
		  	    goto out;
		    }
		    if (bytes_read < BUF_SIZE){
			    read_flag = false; 
		    }
		    bytes_write=vfs_write(fd_out, data, bytes_read,&fd_out->f_pos);
            if (bytes_write!=bytes_read){
                printk("write operation is corrupted.\n");
                retval=-EINTR;
                goto out;
            }
            /*write code for bytes_write != bytes_read*/
/*Enhance this part of the code with vfsstat*/
	    }/*While loop*/
	    set_fs(old_fs);
	    if(fd_out->f_pos !=(*size_of_input)){
            retval = -EINTR;
            goto out;
        }
        printk("Output of file_read---> %ld  and fd_in->f_pos-----> %llu \n",bytes_write,fd_out->f_pos);
	}
/*Deleting the partial  file*/
out:
    putname(file_in);
    putname(file_out);
    if ((fd_out) && (retval!=0)){
        filp_close(fd_out,0);
        printk("deleting the file \n");
        delval = vfs_unlink(fd_out->f_path.dentry->d_parent->d_inode, fd_out->f_path.dentry, NULL);
        if(delval !=0){
            printk("error in deleting the file\n");
        }
    }

/*Cleaning up the code*/
    if (hash_key)//hashed key at kernel land
        kfree(hash_key);
    if (skcipher)
        crypto_free_skcipher(skcipher);
    if (req)
        skcipher_request_free(req);
    if (data)
	    kfree(data); 
    if(fd_in)
        filp_close(fd_in,0);
    if((fd_out) && (retval ==0))
	    filp_close(fd_out,0);
    if(buf)
	    kfree(buf);   
    if(stat)
        kfree(stat);
    if(size_of_input)
        kfree(size_of_input);
    if(key)
        kfree(key);
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
