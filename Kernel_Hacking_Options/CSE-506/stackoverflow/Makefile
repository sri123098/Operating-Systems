obj-m+=sys_stackoverflow.o

INC=/lib/modules/$(shell uname -r)/build/arch/x86/include

all: user kernel

user: user_stackoverflow.c
	gcc -Wall -Werror -I$(INC)/generated/uapi -I$(INC)/uapi user_stackoverflow.c -o user_stackoverflow
kernel:
	make -Wall -Werror -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
	rm -rf user_stackoverflow
