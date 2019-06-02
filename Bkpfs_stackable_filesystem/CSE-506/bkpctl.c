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

int main(int argc, char *argv[])
{
		int c;
		int retval = 0;
		char usage[] = "usage: %s ./bkpctl -[ld:v:r:] FILE_PATH \n";
		int lflag = 0, dflag = 0, vflag = 0, rflag = 0;
		char *kinput = NULL;
		char *input = NULL; 
		int fd;
		extern char *optarg;
		extern int optind, optopt;
		myargs *user = NULL;
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
					lflag += 1;
					if (lflag > 1) 
						goto out;
					input  = argv[optind];
					printf("listing command");
					break;
				case 'd':
					dflag += 1;
					if (dflag > 1)
						goto out;
					kinput = argv[optind - 1];
					input  = argv[optind];
					printf("delete command");
					break;
				case 'v':
					vflag += 1;
					if (vflag > 1)
						goto out;
					kinput = argv[optind - 1];
					input = argv[optind];
					printf("viewing command");
					break;
				case 'r':
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
			user = malloc(sizeof(myargs));
			user->msg = 2;
			retval = ioctl(fd, IOCTL_LIST, user);
			printf("pointer user after ioctl malloc %p \n", user);
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
				retval = -ENOENT;
				printf("no backups availables for this file\n");
			} else {
				c = user->min_version;
				while ( c <= user->max_version) {
					printf(".bkp.%s.%d\n",list,c);
					c++;
				}
			}
		} else if (dflag == 1) {
			user = malloc(sizeof(myargs));
			if (strcmp(kinput, "newest") == 0) {
				user->msg = -1;
			} else if  (strcmp(kinput, "oldest") == 0) {
				user->msg = -2;
			} else if  (strcmp(kinput, "all") == 0) {	
				user->msg = -3;			
			} else {
				printf("Invalid input for delete");
				free(user);
				goto out;
			}		
	
			retval = ioctl(fd, IOCTL_DELETE, user);
			if (user->max_version == 0) {
				retval = -ENOENT;
				printf("Deleted all backups and no backups are \
						 available further to delete\n");
			} else {
				printf("min version %d max version %d \n", user->min_version, user->max_version);	
				printf("deleted the files as per the input\n");
			}
		/*Freeze the above code*/
		} else if (vflag == 1) {
			printf("versioning\n");	
			user = malloc(sizeof(myargs));
			if (strcmp(kinput, "newest") == 0) {
				user->msg = -1;
			} else if  (strcmp(kinput, "oldest") == 0) {
				user->msg = -2;
			} else if  (isnumber(kinput)) {	
				user->msg = atoi(kinput);
				retval = ioctl(fd, IOCTL_LIST, user);
				if ( (user->msg > user->max_version) || (user->msg < user->min_version) ){
					printf("Requested backup file doesn't exist\n");
					printf("Use ./bkpctl -l <filename> to list the versions\n");
					retval = -ENOENT;
					goto out_file;
				}
			} else {
				printf("Invalid input for delete\n");
				free(user);
				goto out;
			}
			retval = ioctl(fd, IOCTL_LIST, user);
			if (user->max_version == 0) {
				retval = -ENOENT;
				printf("Requested back up file doesn't exist\n");
				goto out_file;
			} else {
				user->pos = 0;
				user->bytes_no = 1;
				while(user->bytes_no > 0) {
					retval = ioctl(fd, IOCTL_VIEW, user);
					if (user->bytes_no <=0) 
						break;
					if (user->max_version ==0)
						break;
					printf("%s", (user->bytes_read));
				}
			}
		} else if (rflag == 1) {
			printf("restoring");
			user = malloc(sizeof(myargs));
			if (strcmp(kinput, "newest") == 0) {
				user->msg = -1;
			} else if  (strcmp(kinput, "oldest") == 0) {
				user->msg = -2;
			} else if  (isnumber(kinput)) {	
				user->msg = atoi(kinput);
				retval = ioctl(fd, IOCTL_LIST, user);
				if ( (user->msg > user->max_version) || (user->msg < user->min_version) ){
					printf("Requested backup file doesn't exist\n");
					printf("Use ./bkpctl -l <filename> to list the versions\n");
					retval = -ENOENT;
					goto out_file;
				}
			} else {
				printf("Invalid input for delete");
				free(user);
				goto out;
			}
			retval = ioctl(fd, IOCTL_LIST, user);
			if (user->max_version == 0 ) {
				retval = -ENOENT;
				printf("Requested back up file doesn't exist\n");
			} else {
				retval = ioctl(fd, IOCTL_RESTORE, user);
				printf("Backup file is copied to the original file\n");
			}
		}
		/*clean up code*/
		out_file:
		close(fd);
		if (user) {
			free(user);
		}
		
		return retval ;
		out:
			printf("Pass the correct arguments as stated below:\n");
			printf("%s", usage);
			return retval;
}
