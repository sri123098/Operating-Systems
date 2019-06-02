#!/bin/bash
echo "Testing the user level program for passing the arguments"

./bkpctl -l file > l

if grep --quiet "valid argument for listing" l; then
	echo "Test case for listing the versions is passed"
else
	echo "Test case for listing the versions is failed"
fi


rm -rf l
./bkpctl -d latest file > l

if grep --quiet "valid argument for deleting" l; then
	echo "Test case for deleting the versions is passed"
else
	echo "Test case for deleting the versions is failed"
fi


rm -rf l
./bkpctl -v latest file > l
if grep --quiet "valid argument for viewing" l; then
	echo "Test case for viewing the versions is passed"
else
	echo "Test case for viewing the versions is failed"
fi

rm -rf l
./bkpctl -r latest file > l
if grep --quiet "valid argument for restore" l; then
	echo "Test case for restore the versions is passed"
else
	echo "Test case for restore the versions is failed"
fi


