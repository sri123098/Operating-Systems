#!/bin/bash
echo "Testcase for -h option" 
./xcpenc -h >t1
if grep --quiet usage t1;then 
    echo "-h argument testcase passed"
else
    echo "Testcase failed"
fi
