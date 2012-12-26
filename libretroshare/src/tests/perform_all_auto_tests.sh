#!/bin/sh

# This is the main script for performing all tests automatically.
#   - in order to add new directories, just list them in the $subdirs variable below

echo "****************************************"
echo "*** RetroShare automatic test suite. ***"
echo "****************************************"
echo "Performing all tests on subdirs."
echo "(Some tests take a few minutes. Be patient) "
echo
echo

subdirs="util serialiser dbase upnp general pgp tcponudp"

for dir in $subdirs; do
	echo Tests for directory: $dir
	cd $dir
	./perform_auto_tests.sh 
	cd ..
done

