#!/bin/bash
echo "Testing the encryption and decryption of an empty file"
rm -rf input out3 output
touch input
./xcpenc -p "testing sys call" -e input out3 > t1
sleep 1
./xcpenc -p "testing sys call" -d out3 output > t1
sleep 1
if cmp input output ; then
	echo "------------------------------------------->Test case 0 passed"
else
	echo "xcpenc: input and output files contents DIFFER"
fi
rm -rf input out3 output


touch input
sleep 1
echo "Testing the encrypted file if it is different from the input file"
./xcpenc -p "testing sys call" -e input out3 > t1
if cmp input out3 ; then
	echo "------------------------------------------->Test case 1 failed"
else
    echo "------------------------------------------->Test case 1 passed"
fi
rm -rf input
rm -rf out3
