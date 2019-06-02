#include <asm/unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "/usr/src/hw2-skalluri/fs/bkpfs/shared.h"

#define FALSE (1 == 0)
#define TRUE  (1 == 1)


ioctl_set_msg(int file_desc, char *message)
{
	int ret_val;

	ret_val = ioctl(file_desc, IOCTL_SET_MSG, message);

	if (ret_val < 0) {
		printf("ioctl_set_msg failed:%d\n", ret_val);
		exit(-1);
	}
}

ioctl_get_msg(int file_desc)
{
	int ret_val;
	char message[100];
	
/* 
 * 	 * Warning - this is dangerous because we don't tell
 * 	 	 * the kernel how far it's allowed to write, so it
 * 	 	 	 * might overflow the buffer. In a real production
 * 	 	 	 	 * program, we would have used two ioctls - one to tell
 * 	 	 	 	 	 * the kernel the buffer length and another to give
 * 	 	 	 	 	 	 * it the buffer to fill
 * 	 	 	 	 	 	 	 */
	ret_val = ioctl(file_desc, IOCTL_GET_MSG, message);

	if (ret_val < 0) {
		printf("ioctl_get_msg failed:%d\n", ret_val);
		exit(-1);
	}

	printf("get_msg message:%s\n", message);
}

ioctl_get_nth_byte(int file_desc)
{
	int i;
	char c;

	printf("get_nth_byte message:");

	i = 0;
	do {
		c = ioctl(file_desc, IOCTL_GET_NTH_BYTE, i++);

		if (c < 0) {
			printf
			    ("ioctl_get_nth_byte failed at the %d'th byte:\n",
			     i);
			exit(-1);
		}

		putchar(c);
	} while (c != 0);
	putchar('\n');
}



/*
 * file_isreg check whether input file is regular or not
 * @path: path is the input file path
 */
int file_isreg(const char *path)
{
	struct stat st;
	printf("Checking whether the file is regular or not\n");
	if (stat(path, &st) < 0)
		return -1;
	return S_ISREG(st.st_mode);
}


int main(int argc, char *argv[])
{
		int c;
		int retval = 0;
		int kernel_flag = 0;
		char usage[] = "usage: %s ./bkpctl -[ld:v:r:] FILE_PATH \n";
		int lflag = 0, dflag = 0, vflag = 0, rflag = 0;
		char *kinput = NULL;
		char *input = NULL;//k
		int fd;
		extern char *optarg;
		extern int optind, optopt;
		if (!((argc == 3) || (argc == 4))) {
			printf("File arguments length didn't match");
			goto out;
		}
		/*getopt is used for getting the parameters*/

		while (optind < argc)  {
			c = getopt(argc, argv, "ld:v:r:");
			if (c != -1)  {
				switch (c) {
				case 'l':
					kernel_flag = 1;
					lflag += 1;
					if (lflag > 1) 
						goto out;
					input  = argv[optind];
					printf("listing all the versions");
					break;
				case 'd':
					kernel_flag = 2;
					dflag += 1;
					if (dflag > 1)
						goto out;
					kinput = argv[optind - 1];
					input  = argv[optind];
					printf("deleting the version");
					break;
				case 'v':
					kernel_flag = 4;
					vflag += 1;
					if (vflag > 1)
						goto out;
					kinput = argv[optind - 1];
					input = argv[optind];
					printf("viewing the version");
					break;
				case 'r':
					kernel_flag = 8;
					rflag += 1;
					if (rflag > 1)
						goto out;
					kinput = argv[optind - 1];
					input = argv[optind];
					printf("restore the version");
					break;
				default:
					printf("Invalid flag");
					exit(1);
				}
			} else  {
				optind++;
			}
		}
		printf("Flags which are set in this lflag, dflag, vflag, rflag %d %d %d %d\n", lflag, dflag, vflag, rflag);
		printf("kinput %s input %s", kinput, input);
		if (rflag + vflag + dflag + lflag != 1)
			goto out;
	
		if (file_isreg(input) != 1) {
			printf("please pass the regular file as input");
			goto out;
		}
		
		if ((lflag == 1) && (argc != 3)) {
			printf("not a valid argument for listing");
			goto out;
		} else {
			printf("valid argument for listing");
		}
		if ((dflag == 1) && ( argc != 4)) {
			printf("not a valid argument for deleting");
			goto out;	
		} else {
			printf("valid argument for deleting");
		}
		 
		if ((vflag == 1) && ( argc != 4)) {
			printf("not a valid argument for viewing");
			goto out;	
		} else {
			printf("valid argument for viewing");
		}

		if ((rflag == 1) && ( argc != 4)) {
			printf("not a valid argument for restore");
			goto out;	
		} else {
			printf("valid argument for restore");
		}
		/*integer is the file descriptor*/
		fd = open(input, O_RDONLY);
		if (file_desc < 0) {
                	printf("Can't open device file: %s\n", DEVICE_FILE_NAME);
                	exit(-1);
         	}

		/**
		*Following block is for testing whether the input file is regular or not.
		*You can disable this as this is additional check.
		*/
		
		close(fd);
		return retval ;

		out:
			printf("Pass the correct arguments as stated below:\n");
			printf("%s", usage);
			return retval;
}


