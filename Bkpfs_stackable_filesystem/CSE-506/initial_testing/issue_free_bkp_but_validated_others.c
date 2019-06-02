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
#include "../fs/bkpfs/shared.h"

#define FALSE (1 == 0)
#define TRUE  (1 == 1)


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



/*
 *isnumber checks whether the string is an integer or not
 *@s: pointer to the input string
 */
bool isnumber(char *s) {
	int i=0;
	while(s[i] != '\0') {
		if ( ! isdigit(s[i]))
			return false;
		i++;
	}
	return true;
}

/*
 * validate_input 
 * @kinput: kernel input u can pass to kernel
 */



int main(int argc, char *argv[])
{
		int c;
		int retval = 0;
		int kernel_flag;
		char usage[] = "usage: %s ./bkpctl -[ld:v:r:] FILE_PATH \n";
		int lflag = 0, dflag = 0, vflag = 0, rflag = 0;
		char *kinput = NULL;
		char *input = NULL; 
		int fd;
		extern char *optarg;
		extern int optind, optopt;
		myargs *user = NULL;
		void *msg = NULL;
		char list[30];

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
					printf("listing command");
					break;
				case 'd':
					kernel_flag = 2;
					dflag += 1;
					if (dflag > 1)
						goto out;
					kinput = argv[optind - 1];
					input  = argv[optind];
					printf("delete command");
					break;
				case 'v':
					kernel_flag = 4;
					vflag += 1;
					if (vflag > 1)
						goto out;
					kinput = argv[optind - 1];
					input = argv[optind];
					printf("viewing command");
					break;
				case 'r':
					kernel_flag = 8;
					rflag += 1;
					if (rflag > 1)
						goto out;
					kinput = argv[optind - 1];
					input = argv[optind];
					printf("restore command");
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
		printf("kinput %s input %s \n", kinput, input);
		if (rflag + vflag + dflag + lflag != 1)
			goto out;
	
		if (file_isreg(input) != 1) {
			printf("please pass the regular file as input\n");
			goto out;
		}
		
		if ((lflag == 1) && (argc != 3)) {
			printf("not a valid argument for listing\n");
			retval = -1;
			goto out;
		}
		if ( ((dflag == 1) || (vflag == 1) || (rflag == 1)) && ( argc != 4)) {
			printf("not a valid argument\n");
			goto out;	
		} 
		/*valid_kinput*/

		/*integer is the file descriptor*/
		fd = open(input, O_RDONLY);
		if (fd < 0) {
                	printf("Can't open device file: %s\n", DEVICE_FILE_NAME);
                	exit(-1);
         	}

		if (lflag ==1) {	
			printf("pointer msg before malloc %p \n", msg);
			msg = malloc(sizeof(int));	
			printf("pointer msg before malloc %p \n", msg);
			*((int*)msg) = 2;
			retval = ioctl(fd, IOCTL_LIST, msg);
			printf("pointer msg after ioctl %p \n", msg);
			user = malloc(sizeof(myargs));
			printf("pointer user after ioctl malloc %p \n", user);
			memcpy(user, msg, sizeof(myargs));
			printf("pointer user and msg after memcpy %p , %p \n", user, msg);
			if (strchr(input, '/')) {
				for ( c=strlen(input); c >=0; c--){
					if (input[c] == '/') {
						break;
					}
				}
				strcpy(list, input + c + 1);	
			}
			printf("listing all the versions:\n");
			if (user->max_version == 0) {
				printf("no backups availables for this file\n");
			} else {
				c = user->min_version;
				while ( c <= user->max_version) {
					printf(".bkp.%s.%d\n",list,c);
					c++;
				}
			}
		} else if (dflag == 1) {
			msg = malloc(sizeof(int));
			if (strcmp(kinput, "newest") == 0) {
				*((int*)msg) = -1;
			} else if  (strcmp(kinput, "oldest") == 0) {
				*((int*)msg) = -2;
			} else if  (strcmp(kinput, "all") == 0) {	
				*((int*)msg) = -3;			
			} else {
				printf("Invalid input for delete");
				free(msg);
				goto out;
			}			
			retval = ioctl(fd, IOCTL_DELETE, msg);
			user = malloc(sizeof(myargs));
			memcpy(user, (void *)msg, sizeof(myargs));
			//user = (myargs *)msg;
			if (user->max_version == 0) {
				printf("file deleted all backups and no backups \
						 available further to delete\n");
			} else {
				printf("myarg %d %d \n", user->min_version, user->max_version);	
				printf("deleted the files as per the input\n");
			}
		} else if (vflag == 1) {
			printf("versioning");	
			msg = malloc(sizeof(int));
			if (strcmp(kinput, "newest") == 0) {
				*((int*)msg) = -1;
			} else if  (strcmp(kinput, "oldest") == 0) {
				*((int*)msg) = -2;
			} else if  (isnumber(kinput)) {	
				printf("kinput number %s\n", kinput);	
				*((int*)msg) = atoi(kinput);
			} else {
				printf("Invalid input for delete");
				free(msg);
				goto out;
			}
			/*
 			* Check whether it is a valid version or not by 
 			* checking the min version and then max version
 			*/
			//if (*((int *)msg) > 0) {
			if (true) {
				printf("msg before ioctl in view %d \n", *((int*)msg));
				retval = ioctl(fd, IOCTL_VIEW, msg);
				user = malloc(sizeof(myargs));
				memcpy(user, msg, sizeof(myargs));
			//	user = (myargs *)msg;
				printf("file %d \n", user->min_version);
				printf("file contents %c", *(user->bytes_read));
				printf("Opening the content of the file\n");
				/*
 				* Update the design such that it puts the 
 				* designs-- get the version & then get the data
 				* Design should take the input value and then
 				*/
			}	

		} else if (rflag == 1) {
			printf("restoring");
			/*get the versions and validate the option if it is a number*/
		}
		/*clean up code*/
		close(fd);
		if (msg) {
			free(msg);
		}
		
		/*if (user){
			printf("pointer user and msg after memcpy %p , %p \n", user, msg);
			//printf("pointers msg %p and user  %p \n", msg, user);
			free(user);
			printf("pointer user and msg after memcpy %p , %p \n", user, msg);
			//printf("pointers msg %p and user  %p \n", msg, user);
		}*/
		
		return retval ;
		out:
			// I have to change this in future	
			//	free(msg);
			//free(user);
			printf("Pass the correct arguments as stated below:\n");
			printf("%s", usage);
			return retval;
}
