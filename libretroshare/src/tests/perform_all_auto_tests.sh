#!/bin/sh

echo Performing all tests on subdirs.

subdirs="util serialiser pgp upnp general tcponudp"

for dir in $subdirs; do
	echo Tests for directory: $dir
	cd $dir
	./perform_auto_tests.sh 
	cd ..
done

