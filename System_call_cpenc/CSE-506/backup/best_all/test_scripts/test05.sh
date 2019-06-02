#!/bin/bash
echo "Testing the encryption and decryption of an small file"
rm -rf input
touch input
for i in {1..100}; do     echo "faarqa afs das">> input; done
./xcpenc -p "testing sys call" -e input out3 > t1

./xcpenc -p "testing sys call" -d out3 output > t1

if cmp input output ; then
	echo "------------------------------------------->Test case 0 passed"
else
	echo "xcpenc: input and output files contents DIFFER"
fi
rm -rf out3
rm -rf output



echo "Testing the encryption and decryption of an medium file"
rm -rf input
touch input
for i in {1..1000}; do     echo "faarqa afs das">> input; done
./xcpenc -p "testing sys call" -e input out3 > t1

./xcpenc -p "testing sys call" -d out3 output > t1

if cmp input output ; then
	echo "------------------------------------------->Test case 1 passed"
else
	echo "xcpenc: input and output files contents DIFFER"
fi
rm -rf out3
rm -rf output



echo "Testing the encryption and decryption of an large file"
rm -rf input
touch input
for i in {1..100000}; do     echo "faarqa afs das">> input; done
./xcpenc -p "testing sys call" -e input out3 > t1

./xcpenc -p "testing sys call" -d out3 output > t1

if cmp input output ; then
	echo "------------------------------------------->Test case 2 passed"
else
	echo "xcpenc: input and output files contents DIFFER"
fi
rm -rf out3
rm -rf output
rm -rf input


