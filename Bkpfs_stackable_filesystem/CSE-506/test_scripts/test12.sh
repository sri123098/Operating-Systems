#!/bin/bash
#Empty file test case
#Testcase for null arguments
rm -rf /mnt/bkpfs/a*
#By checking the ls count, I can check whether the backups are seen here or not
touch /mnt/bkpfs/a

if cmp /test/HW2_v6/.bkp.a.1 /mnt/bkpfs/a;then
	echo "******************************"
	echo "Testing for the content of the latest backup file with empty size--------->passed"
	echo "*********************************"
else
	echo "******************************"
	echo "Testing for the content of the latest backup file with empty size------->failed"
	echo "*********************************"
fi


#Clean up code
rm -rf /mnt/bkpfs/a
rm -rf /test/HW2_v6/.bkp.a*
