#!/bin/sh

rm -f ./retroshare_0.5.4-0.*_source.build
rm -f ./retroshare_0.5.4-0.*_source.changes 
rm -f ./retroshare_0.5.4-0.*.tar.gz
rm -f ./retroshare_0.5.4-0.*.diff.gz
rm -f ./retroshare_0.5.4-0.*.dsc 
rm -f *.upload

rm -f *~
find . -name "*~" -exec rm {} \;

