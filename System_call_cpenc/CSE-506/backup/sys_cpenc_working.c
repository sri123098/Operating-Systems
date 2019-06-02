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
asmlinkage extern long (*sysptr)(void *arg);
long int file_read(struct file *file,  char *data, unsigned long size, unsigned long long offset);
long int file_write(struct file *file,  char *data, unsigned long size, unsigned long long offset);


int MD5_hash(char* user_key, int user_keylen, char* digest_hash_key){
		    int retval = 0;
		    //struct shash_desc* desc = (struct shash_desc*)kmalloc(sizeof(struct shash_desc), GFP_KERNEL);
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
                /* write the code for the bytes_read != percentile or BUF_SIZE and bytes_read != bytes_write)*/
                printk("Output of file_read---> %ld  and fd_in->f_pos-----> %llu \n",bytes_write,fd_out->f_pos);
                return retval;
}


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
                if(fd_out->f_pos != (stat->size + 16)){
                    printk(KERN_ERR "issue in writing in encryption");
                    retval=-EINTR;
                    return retval;
                }
                printk("Output of file_read---> %ld  and fd_in->f_pos-----> %llu \n",bytes_write,fd_out->f_pos);
                return retval;
} 

int decrypt_fs(struct file *fd_in, struct file *fd_out, char *data,struct kstat *stat, struct skcipher_request *req , struct scatterlist sg, char *hash_key, char *ivdata){
                ssize_t bytes_read, bytes_write;
                bool read_flag=true;
                int retval=0;
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
                if(fd_out->f_pos != (stat->size - 16)){
                    printk(KERN_ERR "issue in writing in decryption \n");
                    retval=-EINTR;
                    return retval;
                }
                printk("Output of file_read---> %ld  and fd_in->f_pos-----> %llu \n",bytes_write,fd_out->f_pos);
                return retval; 
}



void set_uid_gid(struct file *fd_out){
            printk("current uid %d\n", current_uid().val);
            printk("current gid %d\n", current_gid().val);
            fd_out->f_path.dentry->d_inode->i_uid.val= current_uid().val;
            fd_out->f_path.dentry->d_inode->i_gid.val= current_gid().val;
}

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
		    char ivdata[16] = "itshouldnotcorru";
		    unsigned char *key;
		    struct scatterlist sg;
		    int flag;   
		    struct kstat *stat=NULL;
		    int unlink_val;
            int i;
		    char *hash_key=NULL;
		    myargs *buf;
            buf=kmalloc(sizeof(myargs),GFP_KERNEL);
		    if (!buf){
		        printk(KERN_ERR "Unable to allocate memory.");
		        return -EFAULT;
		    }
		    if (copy_from_user(buf, arg, sizeof(myargs))){
		        retval=-EFAULT;
                goto out;
            }
            key=buf->arg3;
            hash_key=kmalloc(sizeof(char)*16,GFP_KERNEL);
		    if(!hash_key){
                printk(KERN_ERR "Unable to allocate memory.");
                retval= -EFAULT;
                goto out;
            }
            retval=MD5_hash(key, 16, hash_key);
            printk("return val of MD5_hash\n");
            if(retval != 0){
                printk(KERN_ERR "error in hashing the user key\n");
                goto out;
            }    
           flag=buf->flag;
           if (!((flag ==1)||(flag==2)||(flag==4))){
               retval=-EINVAL;
               goto out;
           }

           printk("\nUSER key\n");
            for(i=0;i<16;i++)
                printk(KERN_CONT "%02x", hash_key[i]);
            printk("flag %d",flag);
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
            if (crypto_skcipher_setkey(skcipher, hash_key, 16)) {
                pr_info("key could not be set\n");
                retval = -EAGAIN;
                goto out;
            }
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
            fd_out = filp_open(file_out->uptr, O_WRONLY | O_CREAT, 0777);
            if (!fd_out || IS_ERR(fd_out)) {
                printk("%ld\n", PTR_ERR(fd_out));
                retval= -ENOENT;
                goto out;
            }
            stat=kmalloc(sizeof(struct kstat),GFP_KERNEL);
            if(!stat){
                printk(KERN_ERR "Unable to allocate memory.");
                retval=-EFAULT;
                goto out;
            }
            printk("fd_in->f_cred->uid.val and all others %d %d %d \n", fd_in->f_cred->uid.val,fd_in->f_cred->gid.val,fd_in->f_cred->suid.val);
            printk("fd_out->f_cred->uid.val and all others %d %d %d \n", fd_out->f_cred->uid.val,fd_out->f_cred->gid.val,fd_out->f_cred->suid.val);
            i=vfs_stat(((myargs *)arg)->arg1,stat);
            printk("size of the input file %lld", stat->size);
            printk("Input file %s\n Output file %s\n ",file_in->uptr, file_out->uptr);
            fd_in->f_pos=0;
            fd_out->f_pos=0;
            retval=check_inode(fd_in, fd_out);
            if (retval !=0){
                printk("Both files are same.Inodes Match. Bad arguments. \n");
                retval = -EINVAL ;
                goto out;
            }
            
            set_uid_gid(fd_out);
            
            fd_out->f_path.dentry->d_inode->i_mode=fd_in->f_path.dentry->d_inode->i_mode;
            data=kmalloc(BUF_SIZE,GFP_KERNEL);
            if (!data){
                printk(KERN_ERR "Unable to allocate memory.");
                retval=-EFAULT;
                goto out;
            }
            i=vfs_stat(((myargs *)arg)->arg1,stat);
            if (i !=0){
                retval = -EINTR;
                printk(KERN_ERR "error in vfs-stat");
                goto out;
            }
            printk("size of input file  %lld", stat->size);            
            if(flag & 0x1){
                   retval=encrypt_fs(fd_in,fd_out,data, stat, req, sg, hash_key, ivdata);
                   if (retval !=0)
                       printk("error during encryption\n");
                   goto out;
            }
            else if(flag & 0x2){
                   retval=decrypt_fs(fd_in,fd_out,data, stat, req, sg, hash_key, ivdata);
                   if (retval !=0)
                       printk("error during decryption\n");
                   goto out;
            }
            else if(flag & 0x4){
                  retval=copy_fs(fd_in, fd_out, data,stat);
                  if (retval !=0)
                      printk("error during copy \n");
                  goto out;
            }
            out:
                putname(file_in);
                putname(file_out);
			    if ((fd_out) && (retval != -EINVAL) && (retval!=0)){
			        filp_close(fd_out,0);
			        printk("deleting the file \n");
			        unlink_val = vfs_unlink(fd_out->f_path.dentry->d_parent->d_inode, fd_out->f_path.dentry, NULL);
			        if(unlink_val !=0){
			            printk("error in deleting the file\n");
			        }
			    }
                if (hash_key)
                    kfree(hash_key);
	            if (stat)
	                kfree(stat);
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

