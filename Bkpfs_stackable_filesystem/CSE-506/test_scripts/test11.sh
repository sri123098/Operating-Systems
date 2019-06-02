#!/bin/bash

#Testcase for null arguments
rm -rf /mnt/bkpfs/a*
head -c 4096000 </dev/urandom > my_file
cp my_file /mnt/bkpfs/a
#By checking the ls count, I can check whether the backups are seen here or not
for i in {1..5}; do echo "hi" >> /mnt/bkpfs/a; done
rm -rf del
ls -a /test/HW2_v6/ > del

if cmp /test/HW2_v6/.bkp.a.6 /mnt/bkpfs/a;then
	echo "******************************"
	echo "Testing for the content of the latest backup file with large size--------->passed"
	echo "*********************************"
else
	echo "******************************"
	echo "Testing for the content of the latest backup file with large size------->failed"
	echo "*********************************"
fi


#Clean up code
rm -rf /mnt/bkpfs/a my_file
rm -rf /test/HW2_v6/.bkp.a*
