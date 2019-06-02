#include <asm/unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>
//#include "shared.h"
#ifndef __NR_cpenc
#error cpenc system call not defined
#endif
typedef struct {
	char *arg1;
	char *arg2;
}myargs;
int main(int argc, char *argv[])
{
myargs *args;
int c; 
char usage[] = "usage: %s -p password -[c|e|d] input_fname output_fname \n";
int pflag=0, dflag=0, eflag=0, cflag=0,Cflag=0;
char *input=NULL;//k
char *output=NULL;//k
char *password=NULL;//k
extern char *optarg;
extern int optind, optopt; 
int kernel_flag;//This has to be moved to shared data structure
/*Logic for the below implementation
 *It is picking all the inputs and checking for the flags and then processing them whether they are proper.
 *For example, ./xhw1 -c file1 file2 and ./xhw1 -c file1 file2 -p "password" are valid in this case.
 *So that means it can take either 4 or 6. Rest all the cases, they have follow the usage.
 */

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
args= (myargs *)malloc(sizeof(myargs));
args->arg2=output;
args->arg1=input;
printf("Input %s and Output %s \n", args->arg1, args->arg2);
	int rc;
	void *dummy = (void *)args;
	printf("Size of structure %lu\n", sizeof(args));
	printf("%p Void pointer in user type",dummy);
  	rc = syscall(__NR_cpenc, dummy);
	if (rc == 0)
		printf("syscall returned %d\n", rc);
	else
		printf("syscall returned %d (errno=%d)\n", rc, errno);
	exit(rc);
out:
	printf("Pass the correct arguments as stated below:\n");
	printf("%s",usage);
	return 0;
}
