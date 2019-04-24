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

#define FALSE (1==0)
#define TRUE  (1==1)

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

bool folder_isreg(char *folder){
struct stat s;
  if (stat(folder, &s) == 0 && S_ISDIR(s.st_mode)){
      printf("It is a valid folder\n");
      return TRUE;
  }
  else{
      printf("folder doesnot exist \n");
      return FALSE;
  }
}

/*
 *main program
 */

int main(int argc, char *argv[])
{
myargs *args;
int c;
int i;
//int length;
int retval=0; 
char usage[] = "usage: %s -p password -[c|e|d] input_fname output_fname \n";
int pflag=0, dflag=0, eflag=0, cflag=0, Cflag=0;
char *input=NULL;//k
char *output=NULL;//k
char *password=NULL;
extern char *optarg;
extern int optind, optopt; 
int kernel_flag;
unsigned char hash_password[MD5_DIGEST_LENGTH];
input=malloc(sizeof(sizeof(char)*256));
if(!input){
    printf("Error in memory allocation for input");
    retval= -EFAULT;
    goto out;
}
password=malloc(sizeof(sizeof(char)*50));
if(!password){
    printf("Error in memory allocation for password");
    retval= -EFAULT;
    goto out;
}
output=malloc(sizeof(sizeof(char)*256));
if(!output){
    printf("Error in memory allocation for output");
    retval= -EFAULT;
    goto out;
}

printf("MD5_DIGEST_LENGTH %d",MD5_DIGEST_LENGTH);
if (!((argc==4) || (argc==6)))
	goto out;
//Need to be checked as soon as it is greater than 1, it should return
//It should check for the arguments as well
while (optind < argc) {
	if ((c = getopt(argc, argv, "e:d:c:p:C:")) != -1) {
 		switch(c){
 			case 'e':
 				kernel_flag=1;
				eflag+=1;
				if (eflag > 1)
					goto out;
				input = argv[optind-1];
 				output = argv[optind];
 				break;
 			case 'd':
				kernel_flag=2;
 				dflag+=1;
				if (dflag > 1)
					goto out;
 				input = argv[optind-1];
 				output = argv[optind];
 				break;
 			case 'c':
				kernel_flag=4;
 				cflag+=1;
				if (cflag >1)
					goto out;
 				input = argv[optind-1];
				output = argv[optind];
 				break;
 			case 'p':
 				pflag+=1;
 				if (pflag >1)
					goto out;
                password=argv[optind-1];
//                length=0;
//                if (strlen(argv[optind-1])>50){
//                    printf("please enter the password below 50 characters\n");
//                    goto out;
//                }
//                for(i=0; i < strlen(argv[optind-1]);i++){
//                    if (argv[optind-1][i]!='\n'){
//                        password[length]=argv[optind-1][i];
//                        length++;
//                    }
//                }
//                password[length]='\0';
                //password=argv[optind-1];
 				break;
 			case 'C':
 				Cflag=1;
 				input = argv[optind-1];
 				output = argv[optind];
 				break;
 			default:
 				printf("Invalid flag");
 				exit(1);
 			}//Closing the switch
	}//Closing the if statement
 	else {
 		optind++;  // Skip to the next argument
	 }//Closing the else statement
 	}//Closing the while loop
 printf("Flags which are set in this pflag,cflag,eflag, dflag %d %d %d %d\n", pflag,cflag,eflag, dflag);
 printf("Input file %s, Output file %s, Password %s , Flags p flag %d, c flag %d, e flag %d, d flag %d and C flag %d\n", input, output, password, pflag,cflag,eflag,dflag,Cflag);
if (cflag + eflag + dflag !=1)
	goto out;
if ((kernel_flag!=4) && (pflag!=1))
	goto out;


parse:
args= (myargs *)malloc(sizeof(myargs));
args->arg1=input;
args->arg2=output;
args->flag=kernel_flag;
if((kernel_flag==4) && (pflag!=1)){
   pflag=1;
   password="testing";
}
if (pflag==1)
{
	if(strlen(password)<6){
        printf("Password should be atleast of length 6 characters\n");
        goto out;
    }
    else{
        MD5((const unsigned char*)password, strlen(password), hash_password);
        args->arg3=(char *)hash_password;
        args->length= MD5_DIGEST_LENGTH ;
    }
    for (i=0; i<MD5_DIGEST_LENGTH; i++)
        printf("%02x", hash_password[i]);
}
args->flag=kernel_flag;
    printf("Input %s and Output %s Password %s \n  Kernel_flag %d length %d \n", args->arg1, args->arg2, args->arg3, args->flag, args->length);
    
    int rc;
	void *dummy = (void *)args;
	printf("Size of structure %lu\n", sizeof(args));
	printf("Void pointer %p in user type",dummy);
  	rc = syscall(__NR_cpenc, dummy);
	if (rc == 0)
		printf("syscall returned %d\n", rc);
	else
		printf("syscall returned %d (errno=%d)\n", rc, errno);
	//if(input)
      //  free(input);
    if(args)
		free(args);
//    if(output)
//        free(output);
//    if(password)
//        free(password);
    exit(rc);
out:
    if(input)
        free(input);
    if(output)
        free(output);
    if(password)
        free(password);
	printf("out starts");
	printf("Pass the correct arguments as stated below:\n");
	printf("%s",usage);
	return retval;
}
