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

#define DEVICE_NAME "interface"
//#include "shared.h"
#include "bkpfs.h"
#include <linux/ctype.h>
#include <linux/module.h>
int version = 1;
/*
 * There is no need to lock the bkpfs_super_info's rwsem as there is no
 * way anyone can have a reference to the superblock at this point in time.
 */
static int bkpfs_read_super(struct super_block *sb, void *raw_data, int silent)
{
	int err = 0;
	struct super_block *lower_sb;
	struct path lower_path;
	char *dev_name = (char *) raw_data;
	struct inode *inode;

	if (!dev_name) {
		printk(KERN_ERR
		       "bkpfs: read_super: missing dev_name argument\n");
		err = -EINVAL;
		goto out;
	}

	/* parse lower path */
	err = kern_path(dev_name, LOOKUP_FOLLOW | LOOKUP_DIRECTORY,
			&lower_path);
	if (err) {
		printk(KERN_ERR	"bkpfs: error accessing "
		       "lower directory '%s'\n", dev_name);
		goto out;
	}

	/* allocate superblock private data */
	sb->s_fs_info = kzalloc(sizeof(struct bkpfs_sb_info), GFP_KERNEL);
	if (!BKPFS_SB(sb)) {
		printk(KERN_CRIT "bkpfs: read_super: out of memory\n");
		err = -ENOMEM;
		goto out_free;
	}

	/* set the lower superblock field of upper superblock */
	lower_sb = lower_path.dentry->d_sb;
	atomic_inc(&lower_sb->s_active);
	bkpfs_set_lower_super(sb, lower_sb);

	/* inherit maxbytes from lower file system */
	sb->s_maxbytes = lower_sb->s_maxbytes;

	/*
	 * Our c/m/atime granularity is 1 ns because we may stack on file
	 * systems whose granularity is as good.
	 */
	sb->s_time_gran = 1;

	sb->s_op = &bkpfs_sops;
	sb->s_xattr = bkpfs_xattr_handlers;

	sb->s_export_op = &bkpfs_export_ops; /* adding NFS support */

	/* get a new inode and allocate our root dentry */
	inode = bkpfs_iget(sb, d_inode(lower_path.dentry));
	if (IS_ERR(inode)) {
		err = PTR_ERR(inode);
		goto out_sput;
	}
	sb->s_root = d_make_root(inode);
	if (!sb->s_root) {
		err = -ENOMEM;
		goto out_iput;
	}
	d_set_d_op(sb->s_root, &bkpfs_dops);

	/* link the upper and lower dentries */
	sb->s_root->d_fsdata = NULL;
	err = new_dentry_private_data(sb->s_root);
	if (err)
		goto out_freeroot;

	/* if get here: cannot have error */

	/* set the lower dentries for s_root */
	bkpfs_set_lower_path(sb->s_root, &lower_path);

	/*
	 * No need to call interpose because we already have a positive
	 * dentry, which was instantiated by d_make_root.  Just need to
	 * d_rehash it.
	 */
	d_rehash(sb->s_root);
	if (!silent)
		printk(KERN_INFO
		       "bkpfs: mounted on top of %s type %s\n",
		       dev_name, lower_sb->s_type->name);
	goto out; /* all is well */

	/* no longer needed: free_dentry_private_data(sb->s_root); */
out_freeroot:
	dput(sb->s_root);
out_iput:
	iput(inode);
out_sput:
	/* drop refs we took earlier */
	atomic_dec(&lower_sb->s_active);
	kfree(BKPFS_SB(sb));
	sb->s_fs_info = NULL;
out_free:
	path_put(&lower_path);

out:
	return err;
}

static int bkpfs_parse_options(char *options) {
	int rc = 0;
	int i;
	//extern int version;
	char *p, *k;
	int num = 0;
	if (!options) {
		rc = -EINVAL;
		goto out;
	}
	/*strsep is a macro using string seperation*/
	printk("version in parse_options %d", version);
	
	while ((p = strsep(&options, ",")) != NULL) {
		if (!*p)
			continue;
		if (strncmp(p, "maxver=", 6) == 0) {
			while ((k = strsep(&p, "=")) != NULL) {
				if( isdigit(k[0]) ) {
					for (i = 0; k[i] != '\0'; ++i)
						num = num*10 + k[i] - '0';		
					version = num;
					goto out;
   				}
			}
		} else {		
			printk(KERN_WARNING
			       "%s: bkpfs: unrecognized option [%s]\n",
			       __func__, p);
			rc = -1;
		}
	}
out:
	return rc;

}

struct dentry *bkpfs_mount(struct file_system_type *fs_type, int flags,
			    const char *dev_name, void *raw_data)
{	
	int rc;
	void *lower_path_name = (void *) dev_name;
	/*
 	* raw_data is not being used
 	* write a function for parsing
 	* referred to ecryptfs_mount
 	*/
	rc = bkpfs_parse_options(raw_data);
	if (rc == -1) {
		printk("maxver is not set");
	}
	printk("version %d", version);
	return mount_nodev(fs_type, flags, lower_path_name,
			   bkpfs_read_super);
}

static struct file_system_type bkpfs_fs_type = {
	.owner		= THIS_MODULE,
	.name		= BKPFS_NAME,
	.mount		= bkpfs_mount,
	.kill_sb	= generic_shutdown_super,
	.fs_flags	= 0,
};
MODULE_ALIAS_FS(BKPFS_NAME);

static int __init init_bkpfs_fs(void)
{
	int err;

	pr_info("Registering bkpfs " BKPFS_VERSION "\n");

	err = bkpfs_init_inode_cache();
	if (err)
		goto out;
	err = bkpfs_init_dentry_cache();
	if (err)
		goto out;
	err = register_filesystem(&bkpfs_fs_type);
out:
	if (err) {
		bkpfs_destroy_inode_cache();
		bkpfs_destroy_dentry_cache();
	}
	return err;
}

static void __exit exit_bkpfs_fs(void)
{
	bkpfs_destroy_inode_cache();
	bkpfs_destroy_dentry_cache();
	unregister_filesystem(&bkpfs_fs_type);
	pr_info("Completed bkpfs module unload\n");
}

MODULE_AUTHOR("Erez Zadok, Filesystems and Storage Lab, Stony Brook University"
	      " (http://www.fsl.cs.sunysb.edu/)");
MODULE_DESCRIPTION("Bkpfs " BKPFS_VERSION
		   " (http://bkpfs.filesystems.org/)");
MODULE_LICENSE("GPL");

module_init(init_bkpfs_fs);
module_exit(exit_bkpfs_fs);
