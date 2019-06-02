#!/bin/sh
set -x
make clean
make
lsmod
rmmod sys_softlockup
insmod sys_softlockup.ko
lsmod
