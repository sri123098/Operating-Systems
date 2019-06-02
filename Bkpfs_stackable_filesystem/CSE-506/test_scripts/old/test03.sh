#!/bin/bash
#This script is to check for parsing the different types of the parameters
echo "Testing empty file, small,medium, large sizes" 
touch input2
./xcpenc -p "password" -c input2 out3 >t1
diff -iw input2 out3
if cmp input2 out3 ; then
	echo "------------------------------------------->Test case 0 passed"
else
	echo "xcpenc: input and output files contents DIFFER"
fi
rm -rf input2
rm -rf out3

rm -rf input
touch input
for i in {1..100}; do     echo "testing for different sizes of a file with diff characters">> input; done

./xcpenc -p "password" -c input out3 >t1
diff -iw input out3
if cmp input out3 ; then
	echo "------------------------------------------->Test case 1 passed"
else
	echo "xcpenc: input and output files contents DIFFER"
fi

rm -rf out3

rm -rf input
touch input
for i in {1..1000}; do     echo "testing for different sizes of a file with diff characters">> input; done
./xcpenc -p "password" -c input out3 >t1
diff -iw input out3
if cmp input out3 ; then
	echo "------------------------------------------->Test case 2 passed"
else
	echo "xcpenc: input and output files contents DIFFER"
fi
rm -rf out3

rm -rf input
touch input
for i in {1..100000}; do     echo "testing for different sizes of a file with diff characters">> input; done

./xcpenc -p "password" -c input out3 >t1
diff -iw input out3
if cmp input out3 ; then
	echo "------------------------------------------->Test case 3 passed"
else
	echo "xcpenc: input and output files contents DIFFER"
fi

rm -rf out3
