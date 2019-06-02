#!bin/bash
#This script checks if the input file is non existing and check for the error number 
rm -rf input
./xcpenc -c input out >t 
if grep --quiet "(errno=2)" t;then 
    echo "Non existing file test case passed"
else
    echo "Non existing file test case failed"
fi
