/*
 * Read "len" bytes from "filename" into "buf".
 * "buf" is in kernel space.
 * "data" is in kernel space/




long int file_read(struct file *file,  char *data, unsigned long size, unsigned long long offset) 
{
//ssize_t vfs_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
//check the data types of size_t and ssize_t The mapping is done to 
//typedef unsigned long	__kernel_size_t;
//typedef long		__kernel_ssize_t;
//typedef long long	__kernel_loff_t;
//long int -%ld In old kernel, unsigned char is used
    mm_segment_t oldfs;
    long int ret;
    oldfs = get_fs();
    set_fs(get_ds());
    ret = vfs_read(file, data, size, &offset);
    set_fs(oldfs);
    return ret;
} 





int file_write(struct file *file, unsigned long long offset, unsigned char *data, unsigned int size) 
{
    mm_segment_t oldfs;
    int ret;

    oldfs = get_fs();
    set_fs(get_ds());

    ret = vfs_write(file, data, size, &offset);

    set_fs(oldfs);
    return ret;
}




ssize_t vfs_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	ssize_t ret;

	if (!(file->f_mode & FMODE_WRITE))
		return -EBADF;
	if (!(file->f_mode & FMODE_CAN_WRITE))
		return -EINVAL;
	if (unlikely(!access_ok(VERIFY_READ, buf, count)))
		return -EFAULT;

	ret = rw_verify_area(WRITE, file, pos, count);
	if (!ret) {
		if (count > MAX_RW_COUNT)
			count =  MAX_RW_COUNT;
		file_start_write(file);
		ret = __vfs_write(file, buf, count, pos);
		if (ret > 0) {
			fsnotify_modify(file);
			add_wchar(current, ret);
		}
		inc_syscw(current);
		file_end_write(file);
	}

	return ret;
}



int
wrapfs_read_file(const char *filename, void *buf, int len)
{
    struct file *filp;
    mm_segment_t oldfs;
    int bytes;

    /* Chroot? Maybe NULL isn't right here */
    filp = filp_open(filename, O_RDONLY, 0);
    if (!filp || IS_ERR(filp)) {
	printk("wrapfs_read_file err %d\n", (int) PTR_ERR(filp));
	return -1;  /* or do something else */
    }

    if (!filp->f_op->read) /* better: use vfs_read() */
	return -2;  /* file(system) doesn't allow reads */

    /* now read len bytes from offset 0 */
    filp->f_pos = 0; /* start offset */
    oldfs = get_fs();
    set_fs(KERNEL_DS);
    /* better: use vfs_read() */
    bytes = filp->f_op->read(filp, buf, len, &filp->f_pos);
    set_fs(oldfs);

    /* close the file */
    filp_close(filp, NULL);

    return bytes;
}
