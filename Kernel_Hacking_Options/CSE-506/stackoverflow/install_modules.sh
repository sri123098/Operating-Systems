#!/bin/sh
set -x
make clean
make
lsmod
rmmod sys_stackoverflow
insmod sys_stackoverflow.ko
lsmod
