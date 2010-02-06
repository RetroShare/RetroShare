#!/bin/sh

rm ./retroshare_0.5-1.diff.gz
rm ./retroshare_0.5-1_source.build
rm ./retroshare_0.5.orig.tar.gz
rm ./retroshare_0.5-1_source.changes 
rm ./retroshare_0.5-1.dsc 

rm *~
find . -name "*~" -exec rm {} \;

