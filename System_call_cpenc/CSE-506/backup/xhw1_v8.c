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

args= (myargs *)malloc(sizeof(myargs));
//args is an object which contains two char pointers.
args->arg2= argv[2];
args->arg1=argv[1];
printf("%s input and %s oout\n", args->arg1, args->arg2);
	int rc;
	void *dummy = (void *)args;
	//void *dummy = &args;
	printf("Size of structure %lu\n", sizeof(args));

	printf("%p Void pointer in user type",dummy);
  	rc = syscall(__NR_cpenc, dummy);
	if (rc == 0)
		printf("syscall returned %d\n", rc);
	else
		printf("syscall returned %d (errno=%d)\n", rc, errno);

	exit(rc);
}
