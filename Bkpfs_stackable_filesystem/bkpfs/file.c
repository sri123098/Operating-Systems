/*
 * Copyright (c) 1998-2017 Erez Zadok
 * Copyright (c) 2009	   Shrikar Archak
 * Copyright (c) 2003-2017 Stony Brook University
 * Copyright (c) 2003-2017 The Research Foundation of SUNY
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "bkpfs.h"

static ssize_t bkpfs_read(struct file *file, char __user *buf,
			size_t count, loff_t *ppos)
{
	int err;
	struct file *lower_file;
	struct dentry *dentry = file->f_path.dentry;
	lower_file = bkpfs_lower_file(file);
	err = vfs_read(lower_file, buf, count, ppos);
	/* update our inode atime upon a successful lower read */
	if (err >= 0)
	fsstack_copy_attr_atime(d_inode(dentry),
				file_inode(lower_file));
	return err;
}
/*
 * alloc_add fucntion to create the negative dentry
 * @dentry:file dentry 
 * @lower_path: empty lower_path pointer which get filled
 * @name: Name of the file which you need to create the -ve dentry
 *
 */
struct dentry *alloc_add(struct dentry *dentry, struct path *lower_path,
							 char *name){	  
	struct path lower_parent_path;
	struct dentry *parent;
	struct vfsmount *lower_dir_mnt;
	struct dentry *lower_dir_dentry = NULL;
	struct dentry *lower_dentry;
	int err=0;
	struct qstr this;
	parent = dget_parent(dentry);
	printk("parent dentry %s", parent->d_iname);
	bkpfs_get_lower_path(parent, &lower_parent_path);
	lower_dir_dentry = lower_parent_path.dentry;
	lower_dir_mnt = lower_parent_path.mnt;
	printk("lower_dir_dentry %s", lower_dir_dentry->d_iname);
	this.name = name;
	this.len = strlen(name);
	this.hash = full_name_hash(lower_dir_dentry, this.name, this.len);
	printk("this.name %s", this.name);
	lower_dentry = d_lookup(lower_dir_dentry, &this);
	/*Dont check lower_dentry inode here */
	if (lower_dentry)
	goto setup_lower;
	/*Below is the section for the creation of the -ve dentry*/
	lower_dentry = d_alloc(lower_dir_dentry, &this);
	if (!lower_dentry) {
		err = -ENOMEM;
		goto out;
	}
	d_add(lower_dentry, NULL); /* instantiate and hash(cache addition)*/

setup_lower:
	lower_path->dentry = lower_dentry;
	lower_path->mnt = lower_dir_mnt;
	//lower_path->mnt = mntget(lower_dir_mnt);
	printk("lower_path->dentry %s", lower_path->dentry->d_iname);
	bkpfs_put_lower_path(parent, &lower_parent_path);
	dput(parent);	
	printk("lower_path->dentry %s", lower_path->dentry->d_iname);
 out:
	if (err)
		return ERR_PTR(err);
	return lower_dentry;
}



/*
 * bkp_create function for the creation of the backup file
 * @file:	     file pointer of the upper file system
 * @lower_path_bkp:  path of the lower file system for backup file
 * @lower_file:      lower_file of original file of which backup is needed
 * @bkp_file:	     bkp_file is the new backup file which is created
 * @lower_dentry:    dentry of the backup file
 *
 */

int bkp_create(struct file *file, struct path *lower_path_bkp,
 char *new_bkp_name, struct file *lower_file,
 struct file *bkp_file, struct dentry *lower_dentry) {
	int err_write = 0;
	struct dentry *lower_parent_dentry = NULL; 
	fmode_t fmode_lower_file;	
	lower_dentry=alloc_add(file->f_path.dentry, lower_path_bkp, new_bkp_name);
	printk("lower_dentry name in bkp_create %s", lower_dentry->d_iname);
	lower_parent_dentry = lock_parent(lower_dentry);
	printk("lower_parent_dentry name %s", lower_parent_dentry->d_iname);
	err_write = vfs_create(d_inode(lower_parent_dentry), lower_dentry,
 			 lower_file->f_path.dentry->d_inode->i_mode, true);
	unlock_dir(lower_parent_dentry);
	printk("err_write %d", err_write);
	
	if ((err_write < 0) && !(err_write == -17)) {
		printk("error in vfs_create %d\n", err_write);
		return err_write;
	}
	printk("err_write %d", err_write);
	bkp_file = dentry_open(lower_path_bkp, O_RDWR , current_cred());
	if (err_write == -17) {
		err_write = vfs_truncate( &(bkp_file->f_path), 0);
		printk("err_write if the bkp file exists %d", err_write);	
	}
	fmode_lower_file = lower_file->f_mode;
	lower_file->f_mode = FMODE_READ ;
	/*fpos code needds to be updated*/
	err_write = vfs_copy_file_range(lower_file, 0, bkp_file, 0, lower_file->f_inode->i_size, 0);	
	if (err_write < 0) {
		printk("error in bkp create in vfs_copy");
	}
	lower_file->f_mode = fmode_lower_file;
	fput(bkp_file);
	return err_write;

}

/*
 * bkp_delete_ioctl  function for the deletion of the backup file through ioctl
 *		     It takes care of four things. 
 *		     1)when min and max version are same, removes the attributes
 *		     2)when newest, decrements the max version
 *		     3)when oldest, increments the min version
 *		     4)all, removes all the backup files and remove the attributes	
 *
 * @file:	     file pointer of the upper file system
 * @lower_path_bkp:  path of the lower file system for backup file
 * @lower_file:      lower_file of original file of which backup is needed
 * @karg:	     user arguments from copy from user
 * @lower_dentry:    max version is the latest version of the file as per policy
 * @min_version:     min version is the oldest version of the file as per policy
 */

int bkp_delete_ioctl(struct file *file, struct file *lower_file, 
struct path *lower_path_bkp, int *karg, int *max_version, int *min_version) {
	int set_min_xattr = 0;
	char *max_attr = kmalloc(20, GFP_KERNEL);
	char *min_attr = kmalloc(20, GFP_KERNEL);
	struct dentry *lower_dentry;
	char *new_bkp_name = kmalloc(36, GFP_KERNEL);
	struct dentry *lower_parent_dentry = NULL ;
	int count ;
	strcpy(max_attr, "user.max_version");
	strcpy(min_attr, "user.min_version");
	if ((*max_version - *min_version) == 0 ) {
		printk("both versions are same");
		sprintf(new_bkp_name, ".bkp.%s.%d", file->f_path.dentry->d_iname, *min_version);
		lower_dentry=alloc_add(file->f_path.dentry, lower_path_bkp, new_bkp_name);
		if (lower_dentry->d_inode ==NULL){
			printk("null inode error in minimum version \n");
			set_min_xattr = -ENOENT;
			goto out;
		}
		else {
			printk("Inode exist\n");
			lower_parent_dentry = lock_parent(lower_dentry);		
			set_min_xattr = vfs_unlink(lower_parent_dentry->d_inode, lower_dentry, NULL);	
			unlock_dir(lower_parent_dentry);
			if (set_min_xattr < 0) {
				printk("error in vfs_unlink");
				goto out;
			}
			*min_version = 0 ;
			*max_version = 0 ;
			set_min_xattr = vfs_removexattr(lower_file->f_path.dentry, max_attr);
			set_min_xattr = vfs_removexattr(lower_file->f_path.dentry, min_attr);
			printk("min_version  %d, %d", *min_version, *max_version);
			if (set_min_xattr < 0) {
				printk("error in vfs_setxattr\n");
				goto out;			
			}
		}
	} else if (*karg == -1) {
		printk("max_version delete"); 
		sprintf(new_bkp_name, ".bkp.%s.%d", file->f_path.dentry->d_iname, *max_version);
		lower_dentry=alloc_add(file->f_path.dentry, lower_path_bkp, new_bkp_name);
		if (lower_dentry->d_inode ==NULL){
			printk("null inode error in minimum version \n");
			set_min_xattr = -ENOENT;
			goto out;
		}
		else {
			printk("Inode exist\n");
			lower_parent_dentry = lock_parent(lower_dentry);		
			set_min_xattr = vfs_unlink(lower_parent_dentry->d_inode, lower_dentry, NULL);	
			unlock_dir(lower_parent_dentry);
			if (set_min_xattr < 0) {
				printk("error in vfs_unlink");
				goto out;
			}
			*max_version = *max_version ;
			set_min_xattr = vfs_setxattr(lower_file->f_path.dentry, max_attr, (const void *)max_version , sizeof(int), XATTR_REPLACE );
			printk("min_version  %d, max_version %d", *min_version, *max_version);
			if (set_min_xattr < 0) {
				printk("error in vfs_setxattr\n");
				goto out;			
			}
		}						
	} else if (*karg == -2) {
		printk("min version delete");
		sprintf(new_bkp_name, ".bkp.%s.%d", file->f_path.dentry->d_iname, *min_version);
		lower_dentry=alloc_add(file->f_path.dentry, lower_path_bkp, new_bkp_name);
		if (lower_dentry->d_inode ==NULL){
			printk("null inode error in minimum version \n");
			set_min_xattr = -ENOENT;
			goto out;
		}
		else {
			printk("Inode exist\n");
			lower_parent_dentry = lock_parent(lower_dentry);		
			set_min_xattr = vfs_unlink(lower_parent_dentry->d_inode, lower_dentry, NULL);	
			unlock_dir(lower_parent_dentry);
			if (set_min_xattr < 0) {
				printk("error in vfs_unlink");
				goto out;
			}
			*min_version = *min_version + 1 ;
			set_min_xattr = vfs_setxattr(lower_file->f_path.dentry, min_attr, (const void *)min_version , sizeof(int), XATTR_REPLACE );
			printk("min_version  %d, max_version %d", *min_version, *max_version);
			if (set_min_xattr < 0) {
				printk("error in vfs_setxattr\n");
				goto out;			
			}
		}
	} else if (*karg == -3) {
		for (count = *min_version; count <= *max_version ; count++) { 
			sprintf(new_bkp_name, ".bkp.%s.%d", file->f_path.dentry->d_iname, count);
			lower_dentry=alloc_add(file->f_path.dentry, lower_path_bkp, new_bkp_name);
			if (lower_dentry->d_inode ==NULL){
				printk("null inode error in minimum version \n");
				set_min_xattr = -ENOENT;
				goto out;
			}
			else {
				printk("Inode exist\n");
				lower_parent_dentry = lock_parent(lower_dentry);		
				set_min_xattr = vfs_unlink(lower_parent_dentry->d_inode, lower_dentry, NULL);	
				unlock_dir(lower_parent_dentry);
				if (set_min_xattr < 0) {
					printk("error in vfs_unlink");
					goto out;
				}
			}
		}
		*min_version = 0;
		*max_version = 0;
		set_min_xattr = vfs_removexattr(lower_file->f_path.dentry, max_attr);
		set_min_xattr = vfs_removexattr(lower_file->f_path.dentry, min_attr);
		printk("min_version  %d, max_version %d", *min_version, *max_version);
		if (set_min_xattr < 0) {
			printk("error in vfs_setxattr\n");
			goto out;			
		}
	}
	out:
	kfree(max_attr);
	kfree(min_attr);
	kfree(new_bkp_name);
	return set_min_xattr ;
}
/*
 * bkp_delete	     function for the deletion of the backup file for space retention
 *		     policy. when user is mounting, user need to set version.
 *		     So, when the backups count between the max and min is greater than
 *		     the user set version. It will automatically delete the oldest version
 *		     and increments the min version as per the policy.
 *
 * @file:	     file pointer of the upper file system
 * @lower_path_bkp:  path of the lower file system for backup file
 * @lower_file:      lower_file of original file of which backup is needed
 * @bkp_file:	   bkp_file is the file pointer of the backup file
 * @min_attr:	     minimum attribute
 * @lower_dentry:    lower_dentry is the lower file dentry
 * @max_version:     max version is the latest version of the file as per policy
 * @min_version:     min version is the oldest version of the file as per policy
 */
int bkp_delete(struct file *file, struct path *lower_path_bkp, char *min_attr,
 struct file *lower_file, struct file *bkp_file,
 struct dentry *lower_dentry, int *max_version, int *min_version) {
	int set_min_xattr = 0;
	char *new_bkp_name = kmalloc(36, GFP_KERNEL);
	struct dentry *lower_parent_dentry = NULL ;
	extern int version;
	printk("version retained %d", version);
	if ((*max_version - *min_version) > version) {
		/*unlink the mininmum version and then append the min_version*/
		sprintf(new_bkp_name, ".bkp.%s.%d", file->f_path.dentry->d_iname, *min_version);
		printk("min_version_bkp_name %s", new_bkp_name);
		lower_dentry=alloc_add(file->f_path.dentry, lower_path_bkp, new_bkp_name);
		printk("lower_dentry min version name %s", lower_dentry->d_iname);		
		/*Check whether the lower dentry is existing or not*/
		if (lower_dentry->d_inode ==NULL){
			printk("null inode error in minimum version \n");
			set_min_xattr = -ENOENT;
			goto out;
		}
		else {
			printk("Inode exist\n");
			lower_parent_dentry = lock_parent(lower_dentry);		
			set_min_xattr = vfs_unlink(lower_parent_dentry->d_inode, lower_dentry, NULL);	
			unlock_dir(lower_parent_dentry);
			if (set_min_xattr < 0) {
				printk("error in vfs_unlink");
				goto out;
			}
			*min_version = *min_version + 1;
			set_min_xattr = vfs_setxattr(lower_file->f_path.dentry, min_attr, (const void *)min_version , sizeof(int), XATTR_REPLACE );
			printk("min_version  %d", *min_version);
			if (set_min_xattr < 0) {
				printk("error in vfs_setxattr\n");
				goto out;			
			}
		}		
	}
	out:
	kfree(new_bkp_name);
	return set_min_xattr ;
}

/*
 * attr_get: function to get the attributes
 * @lower_file : lower_file pointer
 * @max_version : pointer to store the max version
 * @min_version : pointer to store the min version
 */

void  attr_get(struct file *lower_file, int *max_version , int *min_version) {
	int get_max_xattr = 0;
	char *max_attr = kmalloc(20, GFP_KERNEL);
	char *min_attr = kmalloc(20, GFP_KERNEL);
	strcpy(max_attr, "user.max_version");
	strcpy(min_attr, "user.min_version");
	printk("get_max_attr before  %d", get_max_xattr);
	get_max_xattr = vfs_getxattr(lower_file->f_path.dentry, max_attr, (void *)max_version, sizeof(int));
	printk("get_max_attr after %d", get_max_xattr);
	if ( (get_max_xattr < 0) || (get_max_xattr == -ENODATA)) {
		*max_version = 0;
		*min_version = 0;
		get_max_xattr = vfs_removexattr(lower_file->f_path.dentry, max_attr);
		get_max_xattr = vfs_removexattr(lower_file->f_path.dentry, min_attr);
	} else {
		*max_version = *max_version - 1;
		get_max_xattr = vfs_getxattr(lower_file->f_path.dentry, min_attr, (void *)min_version, sizeof(int));
	}
	printk("max version and min version in attr_get %d %d", *max_version, *min_version);
	kfree(max_attr);
	kfree(min_attr);
}

/*
 * attr_set:	 function to set the attributes
 * @lower_file:  lower_file pointer
 * @max_attr:	 attribute to be passed either max or min
 * @max_version: pointer to store the min version
 */

int attr_set(struct file *lower_file, char *max_attr, int *max_version) {
	int get_max_xattr = 0, set_max_xattr = 0;
	int set_version = 1;
	get_max_xattr = vfs_getxattr(lower_file->f_path.dentry, max_attr, (void *)max_version, sizeof(int));
	printk("get_max_xattr initial %d", get_max_xattr);	
	if ( (get_max_xattr < 0) || (get_max_xattr == -ENODATA)) {
		set_max_xattr = vfs_setxattr(lower_file->f_path.dentry, max_attr, &set_version , sizeof(int), XATTR_CREATE );
		printk("set_max_xattr	%d", set_max_xattr);
		if (set_max_xattr < 0) {
			printk("error in vfs_setxattr for maximum \n");
			return set_max_xattr;			
		}
		get_max_xattr = vfs_getxattr(lower_file->f_path.dentry, max_attr, (void *)max_version, sizeof(int));
	}
	return get_max_xattr;
}

static ssize_t bkpfs_write(struct file *file, const char __user *buf,
			size_t count, loff_t *ppos)
{
	int err;
	struct file *lower_file;
	struct dentry *dentry = file->f_path.dentry;
	lower_file = bkpfs_lower_file(file);
	err = vfs_write(lower_file, buf, count, ppos);
	/* update our inode times+sizes upon a successful lower write */
	if (err >= 0) {
	fsstack_copy_inode_size(d_inode(dentry),
				file_inode(lower_file));
	fsstack_copy_attr_times(d_inode(dentry),
				file_inode(lower_file));
	}
	return err;
}

/*
 *bkpfs_backup	  main function which is used for the creation as well as the 
		deletion of the oldest back up file as per the policy.
 *@file:	  file pointer of the upper file system
 */
static ssize_t bkpfs_backup(struct file *file)
{
	struct dentry *lower_dentry = NULL ;
	int err_write = 0;
	struct path lower_path_bkp;
	struct file *bkp_file = NULL;
	char *new_bkp_name = kmalloc(36, GFP_KERNEL);
	int *max_version = kmalloc(sizeof(int), GFP_KERNEL);
	int *min_version = kmalloc(sizeof(int), GFP_KERNEL);
	char *max_attr = kmalloc(20, GFP_KERNEL);
	char *min_attr = kmalloc(20, GFP_KERNEL);
	int set_max_xattr = 0, get_max_xattr = 0;
	int get_min_xattr = 0;
	struct file *lower_file;
	lower_file = bkpfs_lower_file(file);
	/*Copying the attributes*/
	strcpy(max_attr, "user.max_version");
	strcpy(min_attr, "user.min_version");
	
	/*getting and setting the attributes*/	
	get_max_xattr = attr_set(lower_file, max_attr, max_version);
	if (get_max_xattr < 0) {
	goto out;	
	}
	get_min_xattr = attr_set(lower_file, min_attr, min_version);
	if (get_min_xattr < 0) {
	goto out;	
	}	
	/* 
	* Creation of the new backup as well as
	* deletion of the old backup is performed over here
	* This function is called in file_release only for regular files
	* and if the file pointer mode is FMODE_WRITE
	* */	
	if (get_max_xattr > 0) {
		printk("get_max_xattr %d", get_max_xattr);
		sprintf(new_bkp_name, ".bkp.%s.%d", file->f_path.dentry->d_iname, *max_version);
		printk("new_bkp_name %s", new_bkp_name);
		err_write = bkp_create(file, &lower_path_bkp, new_bkp_name, lower_file, bkp_file, lower_dentry);
		if (err_write < 0) {
			printk("error in bkp creation err_write %d", err_write);
			goto out;  
		}
		*max_version = *max_version + 1;
		printk("max_version being passed to set attribute after file copy %d", *max_version);
		set_max_xattr = vfs_setxattr(lower_file->f_path.dentry, max_attr, (const void *)max_version , sizeof(int), XATTR_REPLACE );
		printk("set_max_xattr  %d", set_max_xattr);
		if (set_max_xattr < 0) {
			printk("error in vfs_setxattr\n");
			goto out;			
		}
		err_write = bkp_delete(file, &lower_path_bkp, min_attr, lower_file, bkp_file, lower_dentry, max_version, min_version);
		if (err_write < 0) {
			printk("error in bkp deletion as per policy %d", err_write);
			goto out;  
		}		
	} else {
		printk("error in get_max_xattr");
		goto out;
	}
	printk("out side set_max_xattr	%d", set_max_xattr);
	printk("max_version being allocated %d", *max_version);
	printk("out side get_max_xattr %d", get_max_xattr);
out:
	kfree(max_version);
	kfree(min_version);
	kfree(max_attr);
	kfree(min_attr);
	kfree(new_bkp_name);
	return err_write;
}

/*below 3 functions are new*/
struct bkpfs_getdents_callback {
    struct dir_context ctx;
    struct dir_context *caller;
    struct super_block *sb;
    int filldir_called;
    int entries_written;
};

/*
 *bkpfs_filldir: function is updated for not listing the backup files in ls.
 *The following function has been taken from ecryptfs 
 *and modified to hide the files even to use ls -a in the 
 *higher level system. You can see the files by performing
 *ls -a in lower file system.
 */
static int bkpfs_filldir(struct dir_context *ctx, const char *lower_name,
		int lower_namelen, loff_t offset, u64 ino, unsigned int d_type)
{
	struct bkpfs_getdents_callback *buf =
	container_of(ctx, struct bkpfs_getdents_callback, ctx);
	//size_t name_size;
	char *name;
	int rc;
	name = kmalloc(strlen(lower_name), GFP_KERNEL);
	buf->filldir_called++;
	strcpy(name, lower_name);
	rc = strncmp(lower_name, ".bkp.", 5);	
	/*Here I need to make the changes*/
	if (rc != 0) {
		buf->caller->pos = buf->ctx.pos;
		rc = !dir_emit(buf->caller, name, lower_namelen, ino, d_type);
		if (!rc)
			buf->entries_written++;
	} else {
		printk("entering the hiding block\n");
	}
	/*Cleaning the name*/
	kfree(name);
	return rc;
}

static int bkpfs_readdir(struct file *file, struct dir_context *ctx)
{
	int err;
	struct file *lower_file = NULL;
	struct dentry *dentry = file->f_path.dentry;
	/*new 10 lines*/
	struct inode *inode = file_inode(file);
	struct bkpfs_getdents_callback buf = {
	.ctx.actor = bkpfs_filldir,
	.caller = ctx,
	.sb = inode->i_sb,
	};	
	lower_file = bkpfs_lower_file(file);
	err = iterate_dir(lower_file, &buf.ctx);
	ctx->pos = buf.ctx.pos;

	file->f_pos = lower_file->f_pos;
	if (err >= 0)		/* copy the atime */
	fsstack_copy_attr_atime(d_inode(dentry),
				file_inode(lower_file));
	return err;
}

/*
 * bkp_restore	     function to restore the file i.e 
 *		     backup file is being copied to the original file
 * @file:	     file pointer of the upper file system
 * @lower_path_bkp:  path of the lower file system for backup file
 * @lower_file:      lower_file of original file of which backup is needed
 * @kmsg:	     kmsg is myargs pointer contains user commands through copy_from_user
 *
 */



ssize_t bkp_restore(struct file *file, struct file *lower_file, struct path *lower_path_bkp, myargs *kmsg) {
	struct dentry *lower_parent_dentry;
	struct file *bkp_file;
	char *new_bkp_name = kmalloc(36, GFP_KERNEL);
	int ret_val;
	struct file *orig_file = NULL;
	fmode_t fmode_out_file;
	printk("bkp_restore function\n");
	printk("kmsg->msg %d", kmsg->msg);
	if (kmsg->msg == -1) {
		printk("kmsg->max_version %d", kmsg->max_version);
		sprintf(new_bkp_name, ".bkp.%s.%d", file->f_path.dentry->d_iname, (kmsg->max_version));
	} else if (kmsg->msg == -2) {
		sprintf(new_bkp_name, ".bkp.%s.%d", file->f_path.dentry->d_iname, (kmsg->min_version));
	} else if (kmsg->msg > 0) {
		sprintf(new_bkp_name, ".bkp.%s.%d", file->f_path.dentry->d_iname, kmsg->msg);
	} else {
		printk("Invalid argument in view\n");	
		ret_val = -EINVAL;
		goto out;
	}
	
	lower_parent_dentry = dget_parent(lower_file->f_path.dentry);
	ret_val = vfs_path_lookup(lower_parent_dentry, lower_file->f_path.mnt, new_bkp_name, 0, lower_path_bkp);		
	if (ret_val < 0) {
		printk("error in look up");
		goto out;
	} else {
		printk("ret_val %d", ret_val);
		printk("Inode exist\n");
		bkp_file = dentry_open(lower_path_bkp, O_RDWR , current_cred());
		if ((IS_ERR(bkp_file))) {
			printk("error in bkp file\n");
			ret_val = IS_ERR(orig_file);
			goto out;
		}
		printk("bkp_file name %s", bkp_file->f_path.dentry->d_iname);
		orig_file = dentry_open( &lower_file->f_path, O_WRONLY, current_cred());
		if ((IS_ERR(orig_file))) {
			printk("error in lower file\n");
			ret_val = IS_ERR(orig_file);
			fput(bkp_file);
			goto out;
		}
		ret_val = vfs_truncate( &(orig_file->f_path), 0);
		if (ret_val < 0) {
			printk("error in vfs_truncate %d", ret_val);
			fput(bkp_file);
			fput(orig_file);
			goto out;
		}
		if (bkp_file->f_mode & FMODE_READ){
			printk("Input file is proper");
		}
		if (orig_file->f_mode & FMODE_WRITE) {
			printk("Output file are proper");
		}
		printk("lower_file name %s and orig file name %s", lower_file->f_path.dentry->d_iname, orig_file->f_path.dentry->d_iname);
		fmode_out_file = lower_file->f_mode;
		ret_val	= vfs_copy_file_range(bkp_file, 0, orig_file, 0, bkp_file->f_inode->i_size, 0);	
		fput(bkp_file);
		fput(orig_file);
		if (ret_val < 0) {
			printk("error in vfs_copy_file_range %d", ret_val);
			goto out;
		}
	}	
	out:
	kfree(new_bkp_name);
	dput(lower_parent_dentry);
	return ret_val;	
}

/*
 * bkp_read	     function to view the file as per the request
 *		     mainly used for reading the ioctl 
 * @file:	     file pointer of the upper file system
 * @lower_path_bkp:  path of the lower file system for backup file
 * @lower_file:      lower_file of original file of which backup is needed
 * @kmsg:	     kmsg is myargs pointer contains user commands through copy_from_user
 *
 */

ssize_t bkp_read(struct file *file, struct file *lower_file, struct path *lower_path_bkp,  myargs *kmsg) {
	struct dentry *lower_parent_dentry;
	struct file *bkp_file;
	char *new_bkp_name = kmalloc(36, GFP_KERNEL);
	int count = 4096 ;
	char *buf = kmalloc(count, GFP_KERNEL);
	int bytes_read;
	int ret_val;
	printk("bkp_read function\n");
	printk("kmsg->msg %d", kmsg->msg);
	if (kmsg->msg == -1) {
		printk("kmsg->max_version %d", kmsg->max_version);
		sprintf(new_bkp_name, ".bkp.%s.%d", file->f_path.dentry->d_iname, (kmsg->max_version));
	} else if (kmsg->msg == -2) {
		sprintf(new_bkp_name, ".bkp.%s.%d", file->f_path.dentry->d_iname, (kmsg->min_version));
	} else if (kmsg->msg > 0) {
		sprintf(new_bkp_name, ".bkp.%s.%d", file->f_path.dentry->d_iname, kmsg->msg);
	} else {
		printk("Invalid argument in view\n");	
		bytes_read = -EINVAL;
		goto out;
	}
	lower_parent_dentry = dget_parent(lower_file->f_path.dentry);
	ret_val = vfs_path_lookup(lower_parent_dentry, lower_file->f_path.mnt, new_bkp_name, 0, lower_path_bkp);		
	if (ret_val < 0) {
		printk("error in vfs path look up %d", ret_val);
		kmsg->bytes_no = ret_val;
		/*
		*I'm sending this as error in userland
		*/
		goto out;
	} else if(!ret_val) {
		printk("retval %d", ret_val);
		printk("Inode exist\n");
		bkp_file = dentry_open(lower_path_bkp, O_RDWR , current_cred());
		if ((IS_ERR(bkp_file))) {
			printk("error in bkp file\n");
			ret_val = IS_ERR(bkp_file);
			goto out;
		}
		printk("bkp_file name %s", bkp_file->f_path.dentry->d_iname);
		printk("printing the bkp_file f_pos before setting %lld\n", bkp_file->f_pos);  
		bkp_file->f_pos = kmsg->pos ;
		printk("printing the bkp_file f_pos after setting from user %lld\n", bkp_file->f_pos);
		bytes_read = kernel_read(bkp_file, kmsg->bytes_read, count - 1, &(bkp_file->f_pos));
		kmsg->pos = bkp_file->f_pos;
		kmsg->bytes_no = bytes_read;
		printk("bytes_read %d\n", bytes_read);						
		if ((bytes_read == count - 1) || (bytes_read > 0)) {
			*(kmsg->bytes_read + bytes_read) = '\0';	
		} else if (bytes_read < 0) {
			printk("error in kernel_read");
			fput(bkp_file);
			goto out;
		}
		fput(bkp_file);
		if (bytes_read < 0) {
			printk("error in vfs_unlink");
			goto out;
		}
	}
	out:
	kfree(buf);
	dput(lower_parent_dentry);
	kfree(new_bkp_name);
	return bytes_read;	
}

static long bkpfs_unlocked_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	long err = -ENOTTY;
	struct file *lower_file;
	struct path lower_path_bkp;
	myargs *kmsg =	kmalloc(sizeof(myargs), GFP_KERNEL);
	printk("file %s", file->f_path.dentry->d_iname);
	lower_file = bkpfs_lower_file(file);
	printk("lower_file name %s", lower_file->f_path.dentry->d_iname);
	printk("1st UDBG");
	if ( copy_from_user(kmsg, (myargs *)arg, sizeof(myargs))) {
		err = -EFAULT;
		goto out_free;
	} else {
		printk("copy from user is working fine");
		printk("kmsg %d", kmsg->msg);
		printk("kmsg pos %lld", kmsg->pos);	
	}
	if (IOCTL_LIST == cmd) {
		printk("list");
		printk("entering the list command");
		printk("max and min %d, %d", (kmsg->max_version), (kmsg->min_version));
		attr_get(lower_file, &(kmsg->max_version), &(kmsg->min_version));	
		printk("max and min after attr_get %d, %d", (kmsg->max_version), (kmsg->min_version));
	
		if (kmsg->max_version == 0) {
			err = copy_to_user((myargs *)arg, kmsg, sizeof(myargs));
			err = -130;
			goto out_free; 
		}
		err = copy_to_user((myargs *)arg, kmsg, sizeof(myargs));
		printk("error %ld", err);
		if  (err) {
			err = -EFAULT;
			printk("fault in copy to user");
			goto out_free;
		}
	} else if (IOCTL_DELETE == cmd) {
		printk("delete");
		attr_get(lower_file, &(kmsg->max_version), &(kmsg->min_version));
		if (kmsg->max_version == 0) {
			err = copy_to_user((myargs *)arg, kmsg, sizeof(myargs));
			err = -130;
			goto out_free; 
		}
		printk("max and min %d, %d", (kmsg->max_version), (kmsg->min_version));
		err = bkp_delete_ioctl(file, lower_file, &lower_path_bkp, &(kmsg->msg), &(kmsg->max_version) , &(kmsg->min_version));
		printk("bkp_delete_ioctl value %ld", err);
		err = copy_to_user((myargs *)arg, kmsg, sizeof(myargs));
		if (err) {
			err = -EFAULT;
			printk("fault in copy to user");
			goto out_free;
		}
	} else if (IOCTL_VIEW == cmd) {
		printk("view");
		printk("kmsg->pos in view %lld", kmsg->pos);
		attr_get(lower_file, &(kmsg->max_version), &(kmsg->min_version));		
		printk("max version and min version %d, %d", (kmsg->max_version), (kmsg->min_version));
		err = bkp_read(file, lower_file, &lower_path_bkp, kmsg);
		err = copy_to_user((void *)arg, kmsg, sizeof(myargs));
		if (err) {
			err = -EFAULT;
			printk("fault in copy to user");
			goto out_free;		
		}
	} else if (IOCTL_RESTORE == cmd) {
		printk("restore");
		attr_get(lower_file, &(kmsg->max_version), &(kmsg->min_version));
		err = bkp_restore(file, lower_file, &lower_path_bkp, kmsg);
		err = copy_to_user((void *)arg, kmsg, sizeof(myargs));
		if (err) {
			err = -EFAULT;
			printk("fault in copy to user");
			goto out_free;		
		}
	} 

out_free:
	kfree(kmsg);
	/* XXX: use vfs_ioctl if/when VFS exports it */
	if (!lower_file || !lower_file->f_op)
	goto out;
	if (lower_file->f_op->unlocked_ioctl)
	err = lower_file->f_op->unlocked_ioctl(lower_file, cmd, arg);
	/* some ioctls can change inode attributes (EXT2_IOC_SETFLAGS) */
	if (!err)
	fsstack_copy_attr_all(file_inode(file),
				file_inode(lower_file));
out:
	return err;
}

#ifdef CONFIG_COMPAT
static long bkpfs_compat_ioctl(struct file *file, unsigned int cmd,
			unsigned long arg)
{
	long err = -ENOTTY;
	struct file *lower_file;

	lower_file = bkpfs_lower_file(file);
	/* XXX: use vfs_ioctl if/when VFS exports it */
	if (!lower_file || !lower_file->f_op)
	goto out;
	if (lower_file->f_op->compat_ioctl)
	err = lower_file->f_op->compat_ioctl(lower_file, cmd, arg);
out:
	return err;
}
#endif

static int bkpfs_mmap(struct file *file, struct vm_area_struct *vma)
{
	int err = 0;
	bool willwrite;
	struct file *lower_file;
	const struct vm_operations_struct *saved_vm_ops = NULL;

	/* this might be deferred to mmap's writepage */
	willwrite = ((vma->vm_flags | VM_SHARED | VM_WRITE) == vma->vm_flags);

	/*
	* File systems which do not implement ->writepage may use
	* generic_file_readonly_mmap as their ->mmap op.  If you call
	* generic_file_readonly_mmap with VM_WRITE, you'd get an -EINVAL.
	* But we cannot call the lower ->mmap op, so we can't tell that
	* writeable mappings won't work.  Therefore, our only choice is to
	* check if the lower file system supports the ->writepage, and if
	* not, return EINVAL (the same error that
	* generic_file_readonly_mmap returns in that case).
	*/
	lower_file = bkpfs_lower_file(file);
	if (willwrite && !lower_file->f_mapping->a_ops->writepage) {
	err = -EINVAL;
	printk(KERN_ERR "bkpfs: lower file system does not "
		"support writeable mmap\n");
	goto out;
	}

	/*
	* find and save lower vm_ops.
	*
	* XXX: the VFS should have a cleaner way of finding the lower vm_ops
	*/
	if (!BKPFS_F(file)->lower_vm_ops) {
	err = lower_file->f_op->mmap(lower_file, vma);
	if (err) {
		//printk(KERN_ERR "bkpfs: lower mmap failed %d\n", err);
		goto out;
	}
	saved_vm_ops = vma->vm_ops; /* save: came from lower ->mmap */
	}

	/*
	* Next 3 lines are all I need from generic_file_mmap.	I definitely
	* don't want its test for ->readpage which returns -ENOEXEC.
	*/
	file_accessed(file);
	vma->vm_ops = &bkpfs_vm_ops;

	file->f_mapping->a_ops = &bkpfs_aops; /* set our aops */
	if (!BKPFS_F(file)->lower_vm_ops) /* save for our ->fault */
	BKPFS_F(file)->lower_vm_ops = saved_vm_ops;

out:
	return err;
}


static int bkpfs_open(struct inode *inode, struct file *file)
{
	int err = 0;
	struct file *lower_file = NULL;
	struct path lower_path;
	/* don't open unhashed/deleted files */
	if (d_unhashed(file->f_path.dentry)) {
	err = -ENOENT;
	goto out_err;
	}
	file->private_data =
	kzalloc(sizeof(struct bkpfs_file_info), GFP_KERNEL);
	if (!BKPFS_F(file)) {
		err = -ENOMEM;
		goto out_err;
	}
	/* open lower object and link bkpfs's file struct to lower's */
	bkpfs_get_lower_path(file->f_path.dentry, &lower_path);
	lower_file = dentry_open(&lower_path, file->f_flags, current_cred());
	path_put(&lower_path);
	if (IS_ERR(lower_file)) {
		err = PTR_ERR(lower_file);
		lower_file = bkpfs_lower_file(file);
		if (lower_file) {
			bkpfs_set_lower_file(file, NULL);
			fput(lower_file); /* fput calls dput for lower_dentry */
		}
	} else {
		bkpfs_set_lower_file(file, lower_file);
	}
	if (err)
		kfree(BKPFS_F(file));
	else
		fsstack_copy_attr_all(inode, bkpfs_lower_inode(inode));
out_err:
	return err;
}

static int bkpfs_flush(struct file *file, fl_owner_t id)
{
	int err = 0;
	struct file *lower_file = NULL;

	lower_file = bkpfs_lower_file(file);
	if (lower_file && lower_file->f_op && lower_file->f_op->flush) {
	filemap_write_and_wait(file->f_mapping);
	err = lower_file->f_op->flush(lower_file, id);
	}

	return err;
}

/* release all lower object references & free the file info structure */
static int bkpfs_file_release(struct inode *inode, struct file *file)
{
	struct file *lower_file;
	int err_write;
	/* New piece of code being added */
	if ( (S_ISREG(file->f_path.dentry->d_inode->i_mode))
				 && (file->f_mode & FMODE_WRITE)) {
		printk("file is a regular file and in write mode");
		err_write=bkpfs_backup(file);
		printk("err_write %d", err_write);
	}

	/*Below code is already existing*/

	lower_file = bkpfs_lower_file(file);
	if (lower_file) {
	bkpfs_set_lower_file(file, NULL);
	fput(lower_file);
	}
	kfree(BKPFS_F(file));
	//printk("---------------->file release kfree(BKPFS_F(file))\n");
	return 0;
}

static int bkpfs_fsync(struct file *file, loff_t start, loff_t end,
		int datasync)
{
	int err;
	struct file *lower_file;
	struct path lower_path;
	struct dentry *dentry = file->f_path.dentry;

	err = __generic_file_fsync(file, start, end, datasync);
	if (err)
	goto out;
	lower_file = bkpfs_lower_file(file);
	bkpfs_get_lower_path(dentry, &lower_path);
	err = vfs_fsync_range(lower_file, start, end, datasync);
	bkpfs_put_lower_path(dentry, &lower_path);
out:
	return err;
}

static int bkpfs_fasync(int fd, struct file *file, int flag)
{
	int err = 0;
	struct file *lower_file = NULL;

	lower_file = bkpfs_lower_file(file);
	if (lower_file->f_op && lower_file->f_op->fasync)
	err = lower_file->f_op->fasync(fd, lower_file, flag);

	return err;
}

/*
 * Bkpfs cannot use generic_file_llseek as ->llseek, because it would
 * only set the offset of the upper file.  So we have to implement our
 * own method to set both the upper and lower file offsets
 * consistently.
 */
static loff_t bkpfs_file_llseek(struct file *file, loff_t offset, int whence)
{
	int err;
	struct file *lower_file;

	err = generic_file_llseek(file, offset, whence);
	if (err < 0)
	goto out;

	lower_file = bkpfs_lower_file(file);
	err = generic_file_llseek(lower_file, offset, whence);

out:
	return err;
}

/*
 * Bkpfs read_iter, redirect modified iocb to lower read_iter
 */
ssize_t
bkpfs_read_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	int err;
	struct file *file = iocb->ki_filp, *lower_file;

	lower_file = bkpfs_lower_file(file);
	if (!lower_file->f_op->read_iter) {
	err = -EINVAL;
	goto out;
	}

	get_file(lower_file); /* prevent lower_file from being released */
	iocb->ki_filp = lower_file;
	err = lower_file->f_op->read_iter(iocb, iter);
	iocb->ki_filp = file;
	fput(lower_file);
	/* update upper inode atime as needed */
	if (err >= 0 || err == -EIOCBQUEUED)
	fsstack_copy_attr_atime(d_inode(file->f_path.dentry),
				file_inode(lower_file));
out:
	return err;
}

/*
 * Bkpfs write_iter, redirect modified iocb to lower write_iter
 */
ssize_t
bkpfs_write_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	int err;
	struct file *file = iocb->ki_filp, *lower_file;

	lower_file = bkpfs_lower_file(file);
	if (!lower_file->f_op->write_iter) {
	err = -EINVAL;
	goto out;
	}

	get_file(lower_file); /* prevent lower_file from being released */
	iocb->ki_filp = lower_file;
	err = lower_file->f_op->write_iter(iocb, iter);
	iocb->ki_filp = file;
	fput(lower_file);
	/* update upper inode times/sizes as needed */
	if (err >= 0 || err == -EIOCBQUEUED) {
	fsstack_copy_inode_size(d_inode(file->f_path.dentry),
				file_inode(lower_file));
	fsstack_copy_attr_times(d_inode(file->f_path.dentry),
				file_inode(lower_file));
	}
out:
	return err;
}

const struct file_operations bkpfs_main_fops = {
	.llseek		= generic_file_llseek,
	.read		= bkpfs_read,
	.write		= bkpfs_write,
	.unlocked_ioctl	= bkpfs_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= bkpfs_compat_ioctl,
#endif
	.mmap		= bkpfs_mmap,
	.open		= bkpfs_open,
	.flush		= bkpfs_flush,
	.release	= bkpfs_file_release,
	.fsync		= bkpfs_fsync,
	.fasync		= bkpfs_fasync,
	.read_iter	= bkpfs_read_iter,
	.write_iter	= bkpfs_write_iter,
};

/* trimmed directory options */
const struct file_operations bkpfs_dir_fops = {
	.llseek		= bkpfs_file_llseek,
	.read		= generic_read_dir,
	.iterate	= bkpfs_readdir,
	.unlocked_ioctl	= bkpfs_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= bkpfs_compat_ioctl,
#endif
	.open		= bkpfs_open,
	.release	= bkpfs_file_release,
	.flush		= bkpfs_flush,
	.fsync		= bkpfs_fsync,
	.fasync		= bkpfs_fasync,
};
