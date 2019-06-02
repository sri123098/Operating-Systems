#!/bin/bash

#Testcase for null arguments
rm -rf /mnt/bkpfs/a*
echo "hi" > /mnt/bkpfs/a
#By checking the ls count, I can check whether the backups are seen here or not
for i in {1..5}; do echo "hi" >> /mnt/bkpfs/a; done
rm -rf del
ls -a /test/HW2_v6/ > del

if grep --quiet ".bkp.a." del;then
	echo "******************************"
	echo "Testing whether the backups are created in lower file system--------->passed"
	echo "*********************************"
else
	echo "******************************"
	echo "Testing whether the backups are created in lower file system ------->failed"
	echo "*********************************"
fi

rm -rf /mnt/bkpfs/a
rm -rf /test/HW2_v6/.bkp.a*
