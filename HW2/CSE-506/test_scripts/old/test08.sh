#!/bin/bash
#The below arguments are improper argument feeding.
#if print usage is present in the redirected output. I have checked that as well.
rm -rf input
touch input
./xcpenc -p "password" -e input out3 -c input out > t1
if grep --quiet "Pass the correct arguments as stated below:" t1;then
	echo "------------------------------------------->Test case 4 passed"
else
	echo "-------------FAILURE--------------"
fi
rm -rf t1

./xcpenc -p "password" -p input out3 -c input out > t1
if grep --quiet "Pass the correct arguments as stated below:" t1;then
	echo "------------------------------------------->Test case 5 passed"
else
	echo "-------------FAILURE--------------"
fi
rm -rf t1
./xcpenc -p "password" -d input out3 -c input out > t1
if grep --quiet "Pass the correct arguments as stated below:" t1;then
	echo "------------------------------------------->Test case 6 passed"
else
	echo "-------------FAILURE--------------"
fi
rm -rf t1
./xcpenc -p "password" -e input out3 -d input out > t1
if grep --quiet "Pass the correct arguments as stated below:" t1;then
	echo "------------------------------------------->Test case 7 passed"
else
	echo "-------------FAILURE--------------"
fi
rm -rf t1
./xcpenc -p "password" -e input out3 -d input out -c inp out > t1
if grep --quiet "Pass the correct arguments as stated below:" t1;then
	echo "------------------------------------------->Test case 8 passed"
else
	echo "-------------FAILURE--------------"
fi
rm -rf t1
./xcpenc -p "password" -e input out3 -e input out -e inp out > t1
if grep --quiet "Pass the correct arguments as stated below:" t1;then
	echo "------------------------------------------->Test case 9 passed"
else
	echo "-------------FAILURE--------------"
fi
rm -rf t1
./xcpenc -p -p -p -p -p > t1
if grep --quiet "Pass the correct arguments as stated below:" t1;then
	echo "------------------------------------------->Test case 10 passed"
else
	echo "-------------FAILURE--------------"
fi
rm -rf t1



