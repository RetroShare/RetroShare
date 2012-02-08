#!/bin/sh

rm ./retroshare_0.5.3-0.*_source.build
rm ./retroshare_0.5.3-0.*_source.changes 
rm ./retroshare_0.5.3-0.*.tar.gz
rm ./retroshare_0.5.3-0.*.diff.gz
rm ./retroshare_0.5.3-0.*.dsc 

rm *~
find . -name "*~" -exec rm {} \;

