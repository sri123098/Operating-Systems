#!/bin/bash

#Testcase for null arguments
rm -rf /mnt/bkpfs/a*
echo "hi" > /mnt/bkpfs/a
#By checking the ls count, I can check whether the backups are seen here or not
for i in {1..5}; do echo "hi" >> /mnt/bkpfs/a; done
rm -rf del
ls -a /test/HW2_v6/ > del

rm -rf /mnt/bkpfs/b*
echo "hi" > /mnt/bkpfs/b
for i in {1..4}; do echo "hi" >> /mnt/bkpfs/b; done
if cmp /test/HW2_v6/.bkp.a.5 /mnt/bkpfs/b;then
    	echo "******************************"
	echo "Test case for the content of butone backup file --------> passed"
	echo "******************************"
else
    	echo "******************************"
    	echo "Test case for the content of last butone backup file --------> failed"
	echo "******************************"
fi

rm -rf /mnt/bkpfs/b
rm -rf /test/HW2_v6/.bkp.b*

rm -rf /mnt/bkpfs/a
rm -rf /test/HW2_v6/.bkp.a*
