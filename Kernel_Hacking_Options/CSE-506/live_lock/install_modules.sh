#!/bin/sh
set -x
make clean
make
rmmod sys_livelock
insmod sys_livelock.ko
