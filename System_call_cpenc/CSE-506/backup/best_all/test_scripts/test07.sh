#!bin/bash
#This script is for testing the password length
rm -rf input
touch input
./xcpenc -p "pass" -c input out > t1
if grep --quiet "Password should be atleast of length 6 characters" t1;then
	echo "------------------------------------------->Password Length test case passed"
else
	echo "-------------FAILURE--------------"
fi
rm -rf t1
