#!/bin/bash

#Testcase for null arguments
rm -rf /mnt/bkpfs/a*
for i in {1..5}; do echo "isha" >> /mnt/bkpfs/a; done
/usr/src/hw2-skalluri/CSE-506/bkpctl -r newest /mnt/bkpfs/a > temp

for i in {1..5}; do echo "isha" >> test;done

if cmp test /mnt/bkpfs/a;then
	echo "******************************"
	echo "Testing for the content of the newest RESTORE BACKUP  FILE--------->passed"
	echo "*********************************"
else
	echo "******************************"
	echo "Testing for the content of the newest RESTORE BACKUP FILE------>failed"
	echo "*********************************"
fi
rm -rf temp
/usr/src/hw2-skalluri/CSE-506/bkpctl -r 4 /mnt/bkpfs/a > temp
rm -rf test
for i in {1..4}; do echo "isha" >> test;done

if cmp test /mnt/bkpfs/a;then
	echo "******************************"
	echo "Testing for the content of any number RESTORE BACKUP  FILE--------->passed"
	echo "*********************************"
else
	echo "******************************"
	echo "Testing for the content of any number RESTORE BACKUP FILE------>failed"
	echo "*********************************"
fi


rm -rf temp
/usr/src/hw2-skalluri/CSE-506/bkpctl -r oldest /mnt/bkpfs/a > temp
rm -rf test
echo "isha" > test

if cmp test /mnt/bkpfs/a;then
	echo "******************************"
	echo "Testing for the content of the oldest RESTORE BACKUP  FILE--------->passed"
	echo "*********************************"
else
	echo "******************************"
	echo "Testing for the content of the oldest RESTORE BACKUP FILE------>failed"
	echo "*********************************"
fi

rm -rf temp
/usr/src/hw2-skalluri/CSE-506/bkpctl -r 3 /mnt/bkpfs/a > temp
rm -rf test
for i in {1..3}; do echo "isha" >> test;done

if cmp test /mnt/bkpfs/a;then
	echo "******************************"
	echo "Testing for the content of number 3 RESTORE BACKUP  FILE--------->passed"
	echo "*********************************"
else
	echo "******************************"
	echo "Testing for the content of number 3 RESTORE BACKUP FILE------>failed"
	echo "*********************************"
fi


rm -rf temp
/usr/src/hw2-skalluri/CSE-506/bkpctl -r 2 /mnt/bkpfs/a > temp
rm -rf test
for i in {1..2}; do echo "isha" >> test;done

if cmp test /mnt/bkpfs/a;then
	echo "******************************"
	echo "Testing for the content of number 2 RESTORE BACKUP  FILE--------->passed"
	echo "*********************************"
else
	echo "******************************"
	echo "Testing for the content of number 2 RESTORE BACKUP FILE------>failed"
	echo "*********************************"
fi

#Clean up code
rm -rf /mnt/bkpfs/a test temp
rm -rf /test/HW2_v6/.bkp.a*
