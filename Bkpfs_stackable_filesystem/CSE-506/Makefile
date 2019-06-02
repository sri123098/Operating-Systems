
INC=/lib/modules/$(shell uname -r)/build/arch/x86/include

all: ioctl bkpctl

ioctl: bkpctl.c
	gcc -Wall -I$(INC)/generated/uapi -I$(INC)/uapi bkpctl.c -o bkpctl 

bkpctl:
	make -Wall -Werror -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f bkpctl

