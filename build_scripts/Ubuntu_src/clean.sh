#!/bin/sh

rm ./retroshare_0.5.1-0.*_source.build
rm ./retroshare_0.5.1-0.*_source.changes 
rm ./retroshare_0.5.1.orig.tar.gz
rm ./retroshare_0.5.1-0.*.diff.gz
rm ./retroshare_0.5.1-0.*.dsc 

rm *~
find . -name "*~" -exec rm {} \;

