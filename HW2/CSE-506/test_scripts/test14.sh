#!/bin/bash
#for i in {1..500}; do echo "isha" >> temp3; done
#Testcase for null arguments
rm -rf /mnt/bkpfs/a 
rm -rf  temp 
for i in {1..5}; do echo "isha" >> /mnt/bkpfs/a; done
/usr/src/hw2-skalluri/CSE-506/bkpctl -v newest /mnt/bkpfs/a  > temp

#code for removing the first 4 lines
sed -i '1d;2d;3d;4d' temp

if cmp temp /mnt/bkpfs/a;then
	echo "******************************"
	echo "Testing for the content of the newest VIEWING BACKUP  FILE--------->passed"
	echo "*********************************"
else
	echo "******************************"
	echo "Testing for the content of the newest VIEWING BACKUP FILE------>failed"
	echo "*********************************"
fi
sleep 0.3
#Clean up code
rm -rf /mnt/bkpfs/a temp
#umount /mnt/bkpfs
#rmmod bkpfs
#sleep 0.2
#rm -rf /test/HW2_v6
#mkdir /test/HW2_v6
#sleep 0.2
#insmod /usr/src/hw2-skalluri/fs/bkpfs/bkpfs.ko
#mount -t bkpfs -o maxver=5 /test/HW2_v6/ /mnt/bkpfs
