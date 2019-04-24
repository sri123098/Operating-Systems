#!/bin/bash

#Testcase for null arguments
rm -rf /mnt/bkpfs/a*
echo "hi" > /mnt/bkpfs/a
#By checking the ls count, I can check whether the backups are seen here or not
for i in {1..8}; do echo "hi" >> /mnt/bkpfs/a; done
rm -rf del b
touch del
/usr/src/hw2-skalluri/CSE-506/bkpctl -d all /mnt/bkpfs/a > del2

/usr/src/hw2-skalluri/CSE-506/bkpctl -l /mnt/bkpfs/a | grep ".bkp.a" > b

if cmp del b;then
    echo "******************************"
    echo "IOCTL Test case for Deleting the all the backup ---------> passed"
    echo "******************************"
else
    echo "******************************"
    echo "IOCTL Test case for Deleting the all the backup ---------> failed"
    echo "******************************"
fi

#Cleanup code
rm -rf del b del2
rm -rf /mnt/bkpfs/a
rm -rf /test/HW2_v6/.bkp.a*
