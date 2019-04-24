#include <linux/linkage.h>
#include <linux/moduleloader.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include "shared.h"
#include <crypto/hash.h>
#include <linux/crypto.h>
#include <linux/sched.h>
#include <linux/cred.h>
#include <crypto/internal/skcipher.h>
#define BUF_SIZE 4096
//#define EXTRA_CREDIT 1

asmlinkage extern long (*sysptr)(void *arg);
long int file_read(struct file *file,  char *data, unsigned long size, unsigned long long offset);
long int file_write(struct file *file,  char *data, unsigned long size, unsigned long long offset);

/*
 *MD5_hash          performs the hashing of the user passed key in kernel
 *                  It uses crypto_shash_digest function for which the tfm
 *                  is set using the shash_crypto
 *@user_key:        password sent by user
 *@user_keylen:     key len of the password
 *@digest_hash_key: output of the MD5 hashing stored in the digest_hash_key
 */

int MD5_hash(char* user_key, int user_keylen, char* digest_hash_key){
		    int retval = 0;
		    struct crypto_shash* shash_crypto = (struct crypto_shash*)crypto_alloc_shash("md5", 0, CRYPTO_ALG_ASYNC);
		    SHASH_DESC_ON_STACK(desc, tfm);
            if (!shash_crypto || IS_ERR(shash_crypto)){
		        retval = (int)PTR_ERR(shash_crypto);
		        return retval;
		    }
		    desc->tfm = shash_crypto;
		    desc->flags = 0;
            retval = crypto_shash_digest(desc, user_key, user_keylen, digest_hash_key);
            shash_desc_zero(desc);
            crypto_free_shash(shash_crypto);
            return retval;
}
/*
 *copy_fs - perform copy operation of the file pointed by fd_in and writes 
 *          the data into the file pointed by fd_out
 *@fd_in:      file pointer to the input file
 *@fd_out:     file pointer to the output file
 *@data:       data buffer to store the bytes of BUF_SIZE
 *@stat:       kstat structure used to get the size of file
 */


int copy_fs(struct file *fd_in, struct file *fd_out, char *data,struct kstat *stat){
                ssize_t bytes_read, bytes_write;
                bool read_flag=true;
                int retval=0;
                mm_segment_t old_fs;
                old_fs= get_fs();
                set_fs(KERNEL_DS);
                while(read_flag){
                    bytes_read=vfs_read(fd_in, data, BUF_SIZE, &fd_in->f_pos);
                    if (bytes_read < 0){
                        retval = -EINTR;
                        set_fs(old_fs);
                        return retval;
                    }
                    if (bytes_read < BUF_SIZE){
                        read_flag = false; 
                    }
                    bytes_write=vfs_write(fd_out, data, bytes_read,&fd_out->f_pos);
                    if (bytes_read != bytes_write){
                        retval = -EINTR;
                        set_fs(old_fs);
                        return retval;
                    }
                }
                set_fs(old_fs);
                printk("fd_out->f_pos %llu \n", fd_out->f_pos);
                if(fd_out->f_pos != (stat->size)){
                    printk(KERN_ERR "issue in writing in copy");
                    retval=-EINTR;
                }
                printk("Output of file_read---> %ld  and fd_in->f_pos-----> %llu \n",bytes_write,fd_out->f_pos);
                return retval;
}

/*
 *encrypt_fs - perform encryption of the file pointed by fd_in and writes 
 *          the data into the file pointed by fd_out
 *@fd_in:      file pointer to the input file
 *@fd_out:     file pointer to the output file
 *@data:       data buffer to store the bytes of BUF_SIZE
 *@stat:       kstat structure used to get the size of file
 *@sg:         scatterlist used in cipher handle
 *@hash_key:   hashed password at the kernel side used in cipher
 *@ivdata:     Initialization vector used in cipher handle
 */

int encrypt_fs(struct file *fd_in, struct file *fd_out, char *data,struct kstat *stat, struct skcipher_request *req , struct scatterlist sg, char *hash_key, char *ivdata){
                ssize_t bytes_read, bytes_write;
                bool read_flag=true;
                int retval=0;
                mm_segment_t old_fs;
                old_fs= get_fs();
                set_fs(KERNEL_DS);
                bytes_write=vfs_write(fd_out, hash_key, 16,&fd_out->f_pos);
                if (bytes_write < 0){
                    retval = -EINTR;
                    set_fs(old_fs);
                    return retval;
                }
                bytes_write=vfs_write(fd_out, ivdata, 16,&fd_out->f_pos);
                if (bytes_write < 0){
                    retval = -EINTR;
                    set_fs(old_fs);
                    return retval;
                }
                
                while(read_flag){
                    bytes_read=vfs_read(fd_in, data, BUF_SIZE, &fd_in->f_pos);
                    if (bytes_read < 0){
                        retval = -EINTR;
                        set_fs(old_fs);
                        return retval;
                    }
                    sg_init_one(&sg, data, bytes_read);
                    skcipher_request_set_crypt(req, &sg, &sg, bytes_read, ivdata);
                    retval=crypto_skcipher_encrypt(req);
                    if(retval){
                        set_fs(old_fs);
                        return retval;
                    }
                    if (bytes_read < BUF_SIZE){
                        read_flag = false; 
                    }
                    bytes_write=vfs_write(fd_out, data, bytes_read,&fd_out->f_pos);
                    if (bytes_write != bytes_read){
                        retval = -EINTR;
                        set_fs(old_fs);
                        return retval;
                    }
                }
                set_fs(old_fs);
                if(fd_out->f_pos != (stat->size + 32)){
                    printk(KERN_ERR "issue in writing in encryption");
                    retval=-EINTR;
                    return retval;
                }
                printk("Output of file_read---> %ld  and fd_in->f_pos-----> %llu \n",bytes_write,fd_out->f_pos);
                return retval;
} 

/*
 *decrypt_fs - perform decryption of the file pointed by fd_in and writes 
 *          the data into the file pointed by fd_out
 *@fd_in:      file pointer to the input file
 *@fd_out:     file pointer to the output file
 *@data:       data buffer to store the bytes of BUF_SIZE
 *@stat:       kstat structure used to get the size of file
 *@sg:         scatterlist used in cipher handle
 *@hash_key:   hashed password at the kernel side
 */

int decrypt_fs(struct file *fd_in, struct file *fd_out, char *data,struct kstat *stat, struct skcipher_request *req , struct scatterlist sg, char *hash_key){
                ssize_t bytes_read, bytes_write;
                bool read_flag=true;
                int retval=0;
                char ivdata[16];
                mm_segment_t old_fs;
                old_fs= get_fs();
                set_fs(KERNEL_DS);
                bytes_read=vfs_read(fd_in, data, 16,&fd_in->f_pos);
                if (bytes_read < 0){
                    retval = -EINTR;
                    set_fs(old_fs);
                    return retval;
                }
                if (memcmp(data,hash_key,16)!=0){
                    retval=-EACCES;
                    set_fs(old_fs);
                    return retval;
                }
                
                bytes_read=vfs_read(fd_in, data, 16,&fd_in->f_pos);
                if (bytes_read < 0){
                    retval = -EINTR;
                    set_fs(old_fs);
                    return retval;
                }
                memcpy(ivdata,data,16);
                while(read_flag){
                    bytes_read=vfs_read(fd_in, data, BUF_SIZE, &fd_in->f_pos);
                    if (bytes_read < 0){
                        retval = -EINTR;
                        set_fs(old_fs);
                        return retval;
                    }
                    sg_init_one(&sg, data, bytes_read);
                    skcipher_request_set_crypt(req, &sg, &sg, bytes_read, ivdata);
                    retval=crypto_skcipher_decrypt(req);
                    if(retval){
                        set_fs(old_fs);
                        return retval;
                    }
                    if (bytes_read < BUF_SIZE){
                        read_flag = false; 
                    }
                    bytes_write=vfs_write(fd_out, data, bytes_read,&fd_out->f_pos);
                    if (bytes_write != bytes_read){
                        retval = -EINTR;
                        set_fs(old_fs);
                        return retval;
                    }
                }
                set_fs(old_fs);
                if(fd_out->f_pos != (stat->size - 32)){
                    printk(KERN_ERR "issue in writing in decryption \n");
                    retval=-EINTR;
                    return retval;
                }
                printk("Output of file_read---> %ld  and fd_in->f_pos-----> %llu \n",bytes_write,fd_out->f_pos);
                return retval; 
}


/*
 *set_uid_gid - setting the user id and group id of the process
 *@fd_out:file pointer of the output file
 */

void set_uid_gid(struct file *fd_out){
            printk("current uid %d\n", current_uid().val);
            printk("current gid %d\n", current_gid().val);
            fd_out->f_path.dentry->d_inode->i_uid.val= current_uid().val;
            fd_out->f_path.dentry->d_inode->i_gid.val= current_gid().val;
}


/*
 *check_inode -checking the inode of the input file and output file
 *It takes care of symbolic links, hard links and same files
 *@fd_in:file pointer of the input file
 *@fd_out:file pointer of the output file
 */
int check_inode(struct file *fd_in, struct file *fd_out){
            int retval=0;
            unsigned long inode_input  = fd_in->f_path.dentry->d_inode->i_ino ;
            unsigned long inode_output = fd_out->f_path.dentry->d_inode->i_ino ; 
            printk("Inode number of input file %lu\n", fd_in->f_path.dentry->d_inode->i_ino);
            printk("Inode number of output file %lu\n", fd_out->f_path.dentry->d_inode->i_ino);
            if (inode_input == inode_output){
                printk(KERN_ERR "both files inode number match \n");
                retval=-EINVAL;
            }
            return retval;
}

asmlinkage long cpenc(void *arg)
{
		    char *data=NULL;
		    int retval=0;
		    struct file *fd_in, *fd_out;
		    struct filename *file_in, *file_out;
		    struct crypto_skcipher *skcipher = NULL;
		    struct skcipher_request *req = NULL;
		    char *ivdata= NULL;
		    unsigned char *key;
		    struct scatterlist sg;
		    int flag;   
		    struct kstat *stat=NULL;
		    int unlink_val;
            int i;
		    char *hash_key=NULL;
		    myargs *k_arg;
            k_arg=kmalloc(sizeof(myargs),GFP_KERNEL);
		    if (!k_arg){
		        printk(KERN_ERR "Unable to allocate memory.");
		        return -ENOMEM;
		    }
		    if (copy_from_user(k_arg, arg, sizeof(myargs))){
                kfree(k_arg);
                return -EFAULT;
            }
           /*Checking the user space parameters*/
           if((k_arg->input == NULL) || (k_arg->output == NULL) || (k_arg->password == NULL) || (k_arg->flag == 0)) {
                    printk("Invalid Arguments.\n");
                    kfree(k_arg);
                    return -EINVAL;
           }
           /*extra check for missing arguments*/
            flag=k_arg->flag;
            if (!(flag &0x7)){
                   kfree(k_arg);
                   return -EINVAL;
            }
            if((!k_arg->input) || (!k_arg->output) || (!k_arg->password)){
                    printk("Missing arguments\n");
                    kfree(k_arg);        
                    return -EINVAL;
            }
            printk("flag %d",flag);
            printk("input %s", k_arg->input);
            printk("output %s", k_arg->output);
 
            /*Hashing the key in the kernel*/
            key=k_arg->password;
            hash_key=kmalloc(sizeof(char)*16,GFP_KERNEL);
		    if(!hash_key){
                printk(KERN_ERR "Unable to allocate memory.");
                kfree(k_arg);
                return -ENOMEM;
            }
            retval=MD5_hash(key, 16, hash_key);
            printk("return val of MD5_hash\n");
            if(retval != 0){
                printk(KERN_ERR "error in hashing the user key\n");
                kfree(k_arg);
                kfree(hash_key);
                return retval;
            }    

            /*Ensuring the filename which is passed from the user with getname*/
            file_in = getname(((myargs *)arg)->input);
			if (!file_in || IS_ERR(file_in)) {
			    printk("ERROR in getname of input\n");
			    kfree(k_arg);
                kfree(hash_key);
                return PTR_ERR(file_in);
			}
            file_out=getname(((myargs *)arg)->output);
			if (!file_out || IS_ERR(file_in)){
			    printk("ERROR in getname of output\n");
			    kfree(k_arg);
                kfree(hash_key);
                putname(file_in);
                return PTR_ERR(file_out);
			}   
            printk("working fine\n"); 
            
            /*Ensuring the file open is proper*/
            fd_in = filp_open(file_in->name, O_RDONLY, 0);
            if (!fd_in || IS_ERR(fd_in)) {
                printk("%ld\n", PTR_ERR(fd_in));
                kfree(k_arg);
                kfree(hash_key);
                putname(file_in);
                putname(file_out);
                return -ENOENT;
            }
            printk("filp_open issue, %d flag %s %s\n", flag, file_in->name, file_out->name);
            /*Checking whether input is a regular file?*/
            if ((flag & 0x4) && (!S_ISREG(fd_in->f_path.dentry->d_inode->i_mode))) {
                printk("Input is not a regular file");
                retval = -EBADF;
                goto out_state;
            }
            printk("checking whether it is regular file\n");
           /*Ensuring the file open of output is proper*/ 
            fd_out = filp_open(file_out->name, O_WRONLY | O_CREAT, 0777);
            if (!fd_out || IS_ERR(fd_out)) {
                printk("%ld\n", PTR_ERR(fd_out));
                retval= -ENOENT;
                goto out_state;
            }
            printk("checking output file\n");
//             /*Checking whether output is a regular file?*/
            if ((flag & 0x4) && (!S_ISREG(fd_out->f_path.dentry->d_inode->i_mode))){
                printk("Output is not a regular file");
                retval = -EBADF;
                goto out_state1;
            }
            printk("checking output file--2\n");
            printk("Input file %s\n Output file %s\n",file_in->name, file_out->name);
            fd_in->f_pos=0;
            fd_out->f_pos=0;
            /*
             * Checking whether both files are symbolic link 
             * to each other and whether both files inode numbers
             * are same. filp_open is getting through the inode of 
             * the original file
             *
             */
            retval=check_inode(fd_in, fd_out);
            if (retval !=0){
                printk("Both files are same.Inodes Match. Bad arguments. \n");
                retval = -EINVAL ;
                goto out_state1;
            }
            
            set_uid_gid(fd_out);
            
            fd_out->f_path.dentry->d_inode->i_mode=fd_in->f_path.dentry->d_inode->i_mode;
            data=kmalloc(BUF_SIZE,GFP_KERNEL);
            if (!data){
                printk(KERN_ERR "Unable to allocate memory.");
                retval=-ENOMEM;
                goto out_state1;
            }
            
            
            /*Using vfs_stat to get the size of the file*/
            stat=kmalloc(sizeof(struct kstat),GFP_KERNEL);
            if(!stat){
                printk(KERN_ERR "Unable to allocate memory.");
                retval= -ENOMEM;
                goto out_state2;
            }
            i=vfs_stat(((myargs *)arg)->input,stat);
            if (i !=0){
                retval = -EINTR;
                printk(KERN_ERR "error in vfs-stat");
                goto out_state3;
            }
            printk("size of input file  %lld", stat->size);     

          /*Generating the random iv data of 16 bytes*/
           ivdata = kmalloc(16, GFP_KERNEL);
           if (!ivdata) {
                printk(KERN_ERR "Unable to allocate memory.");
                retval=-ENOMEM;
                goto out_state3;
                }
           get_random_bytes(ivdata, 16);
           printk("\nUSER key\n");
            for(i=0;i<16;i++)
                printk(KERN_CONT "%02x", hash_key[i]);

            /*Ensuring the crypto_alloc_skcipher & request handle allocation*/
            skcipher = crypto_alloc_skcipher("ctr(aes)", 0, 0);
            if (IS_ERR(skcipher)) {
                pr_info("could not allocate skcipher handle\n");
                retval= PTR_ERR(skcipher);
                goto out_state4;
            }
            req = skcipher_request_alloc(skcipher, GFP_KERNEL);
            if (!req) {
                pr_info("could not allocate skcipher request\n");
                retval = -ENOMEM;
                goto out_state5;
            }
            if (crypto_skcipher_setkey(skcipher, hash_key, 16)) {
                pr_info("key could not be set\n");
                retval = -EAGAIN;
                goto out_state6;
            }

            /*Actual copy,encyrption and decryption operations*/
            if(flag & 0x1){
                   retval=encrypt_fs(fd_in,fd_out,data, stat, req, sg, hash_key, ivdata);
                   if (retval !=0)
                       printk("error during encryption\n");
            }
            else if(flag & 0x2){
                   retval=decrypt_fs(fd_in,fd_out,data, stat, req, sg, hash_key);
                   if (retval !=0)
                       printk("error during decryption\n");
            }
            else if(flag & 0x4){
                  retval=copy_fs(fd_in, fd_out, data,stat);
                  if (retval !=0)
                      printk("error during copy \n");
            }

            /*
             * Cleaning the state and filp_open 
             * I made the best use of both the worlds in freeing.
             *I have ordered them in First allocate Last Free Policy
             *Removing the output file incase if the file is partially written
             */
	        out_state6:
                skcipher_request_free(req);
            out_state5:
                crypto_free_skcipher(skcipher);
            out_state4:
                kfree(ivdata);
            out_state3:
                kfree(stat);
            out_state2:
                kfree(data);
            out_state1:
                filp_close(fd_out,0);
                if ((retval != -EINVAL) && (retval!=0)){
			        unlink_val = vfs_unlink(fd_out->f_path.dentry->d_parent->d_inode, fd_out->f_path.dentry, NULL);
			        if(unlink_val !=0){
                        printk("error in deleting the file\n");
			        }
			    }                
            out_state:
                kfree(k_arg);
                kfree(hash_key);
                putname(file_in);
                putname(file_out);
                filp_close(fd_in,0);
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

