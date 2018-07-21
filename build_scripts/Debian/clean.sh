#!/bin/sh

rm -f ./retroshare_0.?.?-1_source.build
rm -f ./retroshare_0.?.?-1_source.changes
rm -f ./retroshare_0.?.?-1.tar.gz
rm -f ./retroshare_0.?.?-1.diff.gz
rm -f ./retroshare_0.?.?-1.dsc

rm -f *~
find . -name "*~" -exec rm {} \;

