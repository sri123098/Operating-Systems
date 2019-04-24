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


asmlinkage long cpenc(void *arg)
{
    char *data=NULL;
    bool read_flag = true;
    int retval=0;
    unsigned long inode_input, inode_output;
    struct file *fd_in, *fd_out;
    struct filename *file_in, *file_out;
    ssize_t bytes_read, bytes_write;
    struct crypto_skcipher *skcipher = NULL;
    struct skcipher_request *req = NULL;
    char ivdata[16] = "itshouldnotcorru";
    unsigned char *key=NULL;
    struct scatterlist sg;
    int flag;   
    struct kstat *stat=NULL;
    int i;
    //int hash_check;
    //char *hash_key=NULL;
    mm_segment_t old_fs;
    myargs *buf;
    //hash_key=kmalloc(sizeof(char)*16,GFP_KERNEL);
    stat=kmalloc(sizeof(struct kstat),GFP_KERNEL);
    key=kmalloc(sizeof(char)*16,GFP_KERNEL);
    buf=kmalloc(sizeof(myargs),GFP_KERNEL);
    if (!buf){
        printk(KERN_ERR "Unable to allocate memory.");
        return -EFAULT;
    }
    if (copy_from_user(buf, arg, sizeof(myargs)))
        return -EFAULT;
        key=buf->arg3;
        flag=buf->flag;
        printk("\nUSER key\n");
        for(i=0;i<16;i++)
            printk(KERN_CONT "%02x", key[i]);
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
            if (crypto_skcipher_setkey(skcipher, key, 16)) {
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
            data=kmalloc(BUF_SIZE,GFP_KERNEL);
            if (!data){
                printk(KERN_ERR "Unable to allocate memory.");
                retval=-EFAULT;
                goto out;
            }
            i=vfs_stat(((myargs *)arg)->arg1,stat);
            printk("size of input file  %lld", stat->size);            
            if(flag & 0x1){
                old_fs= get_fs();
                set_fs(KERNEL_DS);
                bytes_write=vfs_write(fd_out, key, 16,&fd_out->f_pos);
                if (bytes_write < 0){
                    retval = -EINTR;
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
                    sg_init_one(&sg, data, bytes_read);
                    skcipher_request_set_crypt(req, &sg, &sg, bytes_read, ivdata);
                    retval=crypto_skcipher_encrypt(req);
                    if(retval){
                        set_fs(old_fs);
                        goto out;
                    }
                    if (bytes_read < BUF_SIZE){
                        read_flag = false; 
                    }
                    bytes_write=vfs_write(fd_out, data, bytes_read,&fd_out->f_pos);
                    if (bytes_write != bytes_read){
                        retval = -EINTR;
                        set_fs(old_fs);
                        goto out;
                    }

                }
                set_fs(old_fs);
                printk("Output of file_read---> %ld  and fd_in->f_pos-----> %llu \n",bytes_write,fd_out->f_pos);
            }
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
                    sg_init_one(&sg, data, bytes_read);
                    skcipher_request_set_crypt(req, &sg, &sg, bytes_read, ivdata);
                    retval=crypto_skcipher_decrypt(req);
                    if(retval){
                        set_fs(old_fs);
                        goto out;
                    }
                    if (bytes_read < BUF_SIZE){
                        read_flag = false; 
                    }
                    bytes_write=vfs_write(fd_out, data, bytes_read,&fd_out->f_pos);
                }
                set_fs(old_fs);
                printk("Output of file_read---> %ld  and fd_in->f_pos-----> %llu \n",bytes_write,fd_out->f_pos);
            }
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
                }
                set_fs(old_fs);
                printk("Output of file_read---> %ld  and fd_in->f_pos-----> %llu \n",bytes_write,fd_out->f_pos);
            }
            out:
                putname(file_in);
                putname(file_out);
//            if (hash_key)
  //              kfree(hash_key);
   if (key)
       kfree(key);
   
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

