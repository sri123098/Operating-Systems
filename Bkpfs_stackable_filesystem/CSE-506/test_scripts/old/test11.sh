#bin/bash
#Testing by passing both the files as same
#if both the files are same, I'm not deleting the file.
rm -rf input i
echo "Hello" > input
ln -s input i
./xcpenc -c input i > t1
echo "Testing for both the files whether they are symbolic link of each other with inode match"
echo "Hello" > output
if cmp input i ; then
	echo "------------------------------------------->Test case for Symbolic link is passed"
else
	echo "xcpenc: input and output files contents DIFFER"
fi

if grep --quiet 22 t1;then 
   echo "------------------------------------------->Test case for Symbolic link is passed"
else
    echo "Testcase for inode is failed"
fi
rm -rf input i
