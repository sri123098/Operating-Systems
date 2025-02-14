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

/*
 * revalidate function is going to revalidate the dentry 
 * in original path mostly used for NFS
 * returns: -ERRNO if error (returned to user)
 *          0: tell VFS to invalidate dentry
 *          1: dentry is valid
 */
static int bkpfs_d_revalidate(struct dentry *dentry, unsigned int flags)
{
	struct path lower_path;
	struct dentry *lower_dentry;
	int err = 1;
	printk("dentry in bkpfs_d_revalidate %s", dentry->d_iname);
	if (flags & LOOKUP_RCU)
		return -ECHILD;

	bkpfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	printk("lower_dentry in bkpfs_d_revalidate %s", lower_dentry->d_iname);
	if (!(lower_dentry->d_flags & DCACHE_OP_REVALIDATE))
		goto out;
	err = lower_dentry->d_op->d_revalidate(lower_dentry, flags);
out:
	bkpfs_put_lower_path(dentry, &lower_path);
	return err;
}

static void bkpfs_d_release(struct dentry *dentry)
{
	/* release and reset the lower paths */
	printk("dentry in bkpfs_d_release %s", dentry->d_iname); 
	bkpfs_put_reset_lower_path(dentry);
	free_dentry_private_data(dentry);
	return;
}

const struct dentry_operations bkpfs_dops = {
	.d_revalidate	= bkpfs_d_revalidate,
	.d_release	= bkpfs_d_release,
};
