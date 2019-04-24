#bin/bash
#Testing by passing both the files as same
#if both the files are same, I'm not deleting the file.
rm -rf input i
echo "Hello" > input
./xcpenc -c input input > t1
echo "Testing for both the files as same with inode match"
echo "Hello" > output
if cmp input output ; then
	echo "------------------------------------------->Test case for inode is passed"
else
	echo "xcpenc: input and output files contents DIFFER"
fi
rm -rf out3
rm -rf output

if grep --quiet 22 t1;then 
   echo "------------------------------------------->Testcase for inode is passed"
else
    echo "Testcase for inode is failed"
fi
