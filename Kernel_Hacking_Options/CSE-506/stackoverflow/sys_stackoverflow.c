#include <linux/init.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/moduleloader.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/time.h>
#include "dummy_struct.h"
asmlinkage extern long (*sysptr_stackoverflow)(void *arg);

void recursion(int num)
{
	char arr[2040];
	
	if (num > 20)
		return;
	sprintf(arr, "%s", "aravindreddyravula");
	pr_info("The value of string is %s and num is %d\n", arr, num);
	recursion(++num);
}
asmlinkage long stackoverflow(void *arg)
{
	recursion(0);
	return 0;
}

static int __init init_sys_stackoverflow(void)
{
	pr_info("installed stackoverflow module!\n");
	if (!sysptr_stackoverflow)
		sysptr_stackoverflow = stackoverflow;
	return 0;
}

static void __exit exit_sys_stackoverflow(void)
{
	if (sysptr_stackoverflow)
		sysptr_stackoverflow = NULL;
	pr_info("removed stackoverflow module!\n");
}

module_init(init_sys_stackoverflow);
module_exit(exit_sys_stackoverflow);
MODULE_LICENSE("GPL");
