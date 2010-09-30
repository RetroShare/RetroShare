#!/bin/sh

echo This script will call valgrind on all executables of the current directory and launch a debugger in case it finds an error. If the script terminate with non error, then the test passes. Press enter when ready...
read tmp

for i in `find . -executable -type f | grep -v valgrind`; do
		echo testing $i
		valgrind --tool=memcheck --db-attach=yes ./$i
done
