#!/bin/bash

#Testcase for null arguments
./xcpenc -i input > t1
if grep --quiet usage t1;then 
    echo "One argument Testcase passed"
else
    echo "Testcase failed"
fi

./xcpenc > t1
if grep --quiet usage t1;then 
    echo "Null arguments Testcase passed"
else
    echo "Testcase failed"
fi
