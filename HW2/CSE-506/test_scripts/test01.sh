#!/bin/bash

#Testcase for null arguments
rm -rf /mnt/bkpfs/a*
echo "hi" > /mnt/bkpfs/a
#By checking the ls count, I can check whether the backups are seen here or not
for i in {1..5}; do echo "hi" >> /mnt/bkpfs/a; done
rm -rf del
ls -a /mnt/bkpfs/* > del

if grep --quiet ".bkp.a." del;then
	echo "******************************"
	echo "Testing for the backups whether the backups are hiding--------->failed"
	echo "*********************************"
else
	echo "******************************"
	echo "Testing for the backups whether the backups are hiding--------->passed"
	echo "*********************************"
fi

rm -rf /mnt/bkfs/a*
rm -rf /test/HW2_v6/.bkp.a*

