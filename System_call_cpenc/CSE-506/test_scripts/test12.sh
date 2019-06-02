#This script tests for non existing file during the decryption and encryption as it requires password.
#!/bin/bash 
rm -rf input
./xcpenc -e input out -p "passwprd" >t 
if grep --quiet "(errno=2)" t;then 
    echo "Non existing file test case for encryption passed"
else
    echo "Non existing file test case failed"
fi

rm -rf input
./xcpenc -d input out -p "password" >t 
if grep --quiet "(errno=2)" t;then 
    echo "Non existing file test case for decryption passed"
else
    echo "Non existing file test case failed"
fi



#I have additonally tested for partial created file with the help of vfs_unlink
echo "Testcase for partially deleted file"
echo "Test case for vfs unlink is verified by changing the sys_cpenc.c file and then setting the checking for the
deletion of a file. Please check the check_vfs_unlink. So, it will be cleaning the output file if some error occurs."
