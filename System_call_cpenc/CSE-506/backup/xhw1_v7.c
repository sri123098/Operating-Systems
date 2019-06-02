#include <asm/unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "shared.h"
#ifndef __NR_cpenc
#error cpenc system call not defined
#endif

int main(int argc, const char *argv[])
{
struct myargs args;
strcpy(args.arg2, argv[2]);
strcpy(args.arg1, argv[1]);
	int rc;
	void *dummy = (void *)&args;
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
