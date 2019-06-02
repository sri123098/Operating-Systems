#ifndef SHARED_H
#define SHARED_H
typedef struct {
	int msg;
	long long pos;
	int min_version;
	int max_version;
	int bytes_no;
	char bytes_read[4096];
}myargs;

#include <linux/ioctl.h>
#define MAJOR_NUM 'C'
#define IOCTL_LIST _IOWR(MAJOR_NUM, 1, myargs *)
#define IOCTL_DELETE _IOWR(MAJOR_NUM, 2, myargs *)
#define IOCTL_VIEW _IOWR(MAJOR_NUM, 3, myargs *)
#define IOCTL_RESTORE _IOWR(MAJOR_NUM, 4, myargs *)
#define DEVICE_FILE_NAME "interface"
#endif
