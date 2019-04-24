#!/bin/bash

#Testcase for null arguments
rm -rf /mnt/bkpfs/a*
echo "hi" > /mnt/bkpfs/a
#By checking the ls count, I can check whether the backups are seen here or not
for i in {1..10}; do echo "hi" >> /mnt/bkpfs/a; done
rm -rf del
ls -a /test/HW2_v6/ | grep "bkp.a" | wc -l  > del

rm -rf b
echo 5 > b
if cmp del b;then
    echo "******************************"
    echo "Test case for Number of backups ---------> passed"
    echo "******************************"
else
    echo "******************************"
    echo "Test case for Number of backups ---------> failed"
    echo "******************************"
fi

rm -rf del

rm -rf /mnt/bkpfs/a
rm -rf /test/HW2_v6/.bkp.a*
