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
#include <openssl/md5.h>
#include "shared.h"
#ifndef __NR_cpenc
#error cpenc system call not defined
#endif

#define FALSE (1 == 0)
#define TRUE  (1 == 1)

/*
 *Checking whether the file is regular or not
 */
int file_isreg(const char *path) {
    struct stat st;
    printf("Checking whether the file is regular or not\n");
    if (stat(path, &st) < 0)
        return -1;
    return S_ISREG(st.st_mode);
}

/*
 *Checking whether the folder is regular or not
 */

bool folder_isreg(char *folder) {
struct stat s;
  if (stat(folder, &s) == 0 && S_ISDIR(s.st_mode)) {
      printf("It is a valid folder\n");
      return TRUE;
  }
  else {
      printf("folder doesnot exist \n");
      return FALSE;
  }
}

/* 
 * To check whether the file exists in user land or not
 * Bypass this while checking the system call till parse
 *access() succeeds and shall return 0; otherwise, -1 shall be returned and errno shall be set to indicate the error
 *As discussed in the class, I have implemented the userlevel checks for input and output
 *Especially for the output, I need to check whether the output folder exists or not and then check for the permissions of the output file to write using W_OK
 *
 */
int validate_input(char *input) {
        if (file_isreg(input) == 1) {
	        printf("Input File is Regular.Checking for the access permissions\n");
	        if (access(input,R_OK) == 0) {
		    	printf("Read access is available for input file \n");
                return 0;
			}else {
			    printf("Read access is not allowed\n");
			    return -EPERM;
			}
		}else {
		    printf("File doesn't exists.(errno=2)\n");
	 	    return -ENOENT;
		}

}

int check_folder(char *output) {
		        printf("Entering the folder block %s %lu \n", output, strlen(output));
		        long unsigned int fold_ptr;
	            int l;
		        for(l = strlen(output)-1;l > -1; l--)
	             {if (output[l] == '/') {
                        fold_ptr = l;
		                break;
		         }
	            }//Closing the for loop
	            char str[sizeof((fold_ptr+1)*sizeof(char))];
	            strncpy(str, output, fold_ptr);
                printf("Folder to be checked %s\n",str);
	            if (folder_isreg(str))
	             {printf("Output file relative directory exist and continue what you are doing\n");
                	if (access(str,W_OK) == 0) {
			            printf("Write access of the output relative folder is available\n");
			            return 0;
			        }else {
			            printf("Permissions to output relative folder is not present\n");
			            return -EPERM;
			        }
		        }//Closing the folder_isreg if block
	            else {   
			        printf("Output file relative directory doesn't exist\n");
			        return -ENOENT;
	            }//Closing the folder_isreg else block
}


int validate_output(char *output) {
    int retval = 0;
    if(strchr(output,'/')) {
        retval = check_folder(output);
        return retval;
        }
    else {
		   if (file_isreg(output) == 1) {
			    if ( access( output, W_OK ) == 0) {
				    printf("You can overwrite the existing output file\n");
				    return 0;
               } else {
		            printf("Write access to the output file is not present.\n");
			    	retval = -EPERM;
			        return retval;
                }
		  } else {
			    if(access("./",W_OK) == 0) {
				    printf("You can create the output file in the current folder\n");
			        return 0;
				} else {
                    printf("Write access to the output file is not present.\n");
			    	retval = -EPERM;
			    	return retval;
			    }
		   }
	   }
}

int main(int argc, char *argv[])
 {
myargs *args;
int c;
int i;
int retval = 0;
char usage[] = "usage: %s -p password -[c|e|d] input_fname output_fname \n";
int pflag = 0, dflag = 0, eflag = 0, cflag = 0, Cflag = 0;
char *input = NULL;//k
char *output = NULL;//k
char *password = NULL;//k
extern char *optarg;
extern int optind, optopt;
int kernel_flag;
unsigned char hash_password[MD5_DIGEST_LENGTH];
printf("MD5_DIGEST_LENGTH %d", MD5_DIGEST_LENGTH);
//This has to be moved to shared data structure
/*Logic for the below implementation
 *It is picking all the inputs and checking for the flags and then processing them whether they are proper.
 *For example, ./xhw1 -c file1 file2 and ./xhw1 -c file1 file2 -p "password" are valid in this case.
 *So that means it can take either 4 or 6. Rest all the cases, they have follow the usage.
 */
if (!((argc == 4) || (argc == 6)))
	goto out;
//Need to be checked as soon as it is greater than 1, it should return
//It should check for the arguments as well
while (optind < argc)  {
	if ((c = getopt(argc, argv, "e:d:c:p:C:")) != -1)  {
 		switch (c) {
 			case 'e':
 				kernel_flag = 1;
				eflag += 1;
				if (eflag > 1)
					goto out;
				input = argv[optind - 1];
 				output = argv[optind];
 				break;
 			case 'd':
				kernel_flag = 2;
 				dflag += 1;
				if (dflag > 1)
					goto out;
 				input = argv[optind - 1];
 				output = argv[optind];
 				break;
 			case 'c':
				kernel_flag = 4;
 				cflag += 1;
				if (cflag > 1)
					goto out;
 				input = argv[optind - 1];
				output = argv[optind];
 				break;
 			case 'p':
 				pflag += 1;
 				if (pflag > 1)
					goto out;
				password = argv[optind - 1];
 				break;
 			case 'C':
 				Cflag = 1;
 				input = argv[optind - 1];
 				output = argv[optind];
 				break;
 			default:
 				printf("Invalid flag");
 				exit(1);
 			}
	}
 	else  {
 		optind++;  // Skip to the next argument
	 }
 	}
 printf("Flags which are set in this pflag, cflag, eflag, dflag %d %d %d %d\n", pflag, cflag, eflag, dflag);
 printf("Input file %s, Output file %s, Password %s , Flags p flag %d, c flag %d, e flag %d, d flag %d and C flag %d\n", input, output, password, pflag, cflag, eflag, dflag, Cflag);
if (cflag + eflag + dflag != 1)
	goto out;
if ((kernel_flag != 4) && (pflag != 1))
	goto out;
/**
 *Following 2 if blocks is to validate the input file and ouput file 
 *I have tested by bypassing these two if blocks as well for checking whether it is a regular file or not
 *
 */
retval = validate_input(input);
if (retval != 0)
    goto out;
retval = validate_output(output);
if (retval != 0)
    goto out;

args = (myargs *)malloc(sizeof(myargs));
args->input = input;
args->output = output;
args->flag = kernel_flag;
if ((kernel_flag == 4) && (pflag != 1)) {
   pflag = 1;
   password = "testing";
}

if (pflag == 1)
 {
	if (strlen(password) < 6) {
        printf("Password should be atleast of length 6 characters\n");
        goto out;
    }
    else {
        MD5((const unsigned char *)password, strlen(password), hash_password);
        args->password = (char *)hash_password;
        args->length = MD5_DIGEST_LENGTH ;
    }
    for (i = 0; i < MD5_DIGEST_LENGTH; i++)
        printf("%02x", hash_password[i]);
}
args->flag = kernel_flag;
    printf("Input %s and Output %s Password %s Kernel_flag %d length %d \n", args->input, args->output, args->password, args->flag, args->length);
	int rc;
	void *dummy = (void *)args;
    rc = syscall(__NR_cpenc, dummy);
	if (rc == 0)
		printf("syscall returned %d\n", rc);
	else
		printf("syscall returned %d (errno=%d)\n", rc, errno);
	if (args)
		free(args);
	exit(rc);
out:
	printf("out starts");
	printf("Pass the correct arguments as stated below:\n");
	printf("%s", usage);
	return retval;
}
