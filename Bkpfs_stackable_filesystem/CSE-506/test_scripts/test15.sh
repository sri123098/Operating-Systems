#!/bin/bash
#for i in {1..500}; do echo "isha" >> temp3; done
#Testcase for null arguments
rm -rf /mnt/bkpfs/b 
rm -rf  temp 
for i in {1..5}; do echo "isha" >> /mnt/bkpfs/b; done
/usr/src/hw2-skalluri/CSE-506/bkpctl -v oldest /mnt/bkpfs/b  > temp

#code for removing the first 4 lines
sed -i '1d;2d;3d;4d' temp
echo "isha" > temp2
if cmp temp temp2;then
	echo "******************************"
	echo "Testing for the content of the oldest VIEWING BACKUP  FILE--------->passed"
	echo "*********************************"
else
	echo "******************************"
	echo "Testing for the content of the oldest VIEWING BACKUP FILE------>failed"
	echo "*********************************"
fi
sleep 0.3
#Clean up code
rm -rf /mnt/bkpfs/b temp temp2
#umount /mnt/bkpfs
#rmmod bkpfs
#sleep 0.2
#rm -rf /test/HW2_v6
#mkdir /test/HW2_v6
#insmod /usr/src/hw2-skalluri/fs/bkpfs/bkpfs.ko
#mount -t bkpfs -o maxver=5 /test/HW2_v6/ /mnt/bkpfs

