#!/bin/bash
#This script is for checking different way to parse the arguments
rm -rf input 
touch input
for i in {1..100};do echo "Testing for arguments parsing">> input; done
echo "Test1 ./xcpenc -p "password" -c input out3"
./xcpenc -p "password" -c input out3 >t1
diff -iw input out3
if cmp input out3 ; then
	echo "------------------------------------------->Test case 1 passed"
else
	echo "xcpenc: input and output files contents DIFFER"
fi

rm -rf out3

echo "Test2 ./xcpenc -c input out3 -p "password""
./xcpenc -c input out3 -p "password" > t1
diff -iw input out3
if cmp input out3 ; then
	echo "------------------------------------------->Test case 2 passed"
else
	echo "xcpenc: input and output files contents DIFFER"
fi

rm -rf out3

echo "Test3 ./xcpenc -c input out3"
./xcpenc -c input out3 > t1
diff -iw input out3
if cmp input out3 ; then
	echo "------------------------------------------->Test case 3 passed"
else
	echo "xcpenc: input and output files contents DIFFER"
fi
rm -rf out3


