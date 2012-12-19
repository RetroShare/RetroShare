#!/bin/sh

printresult() {
	if test "$?" = "0"; then
		echo ' 'PASSED
	else
		echo *FAILED*
	fi
}

# Warning: printresult needs to be called before anything else because it contains the 
# result of the call to the test program, until the next command.

exes="sha1_test"

for exe in $exes; do
 ./$exe  > /dev/null 2>&1 ; result=`printresult`; echo "-- $exe \t test :" $result ;
done



