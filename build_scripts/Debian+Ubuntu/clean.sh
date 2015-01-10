#!/bin/sh

rm -f ./libssh-0.5.4.tar.gz.*
rm -f ./retroshare06_0.6.0-0.*_source.build
rm -f ./retroshare06_0.6.0-0.*_source.changes 
rm -f ./retroshare06_0.6.0-0.*.tar.gz
rm -f ./retroshare06_0.6.0-0.*.diff.gz
rm -f ./retroshare06_0.6.0-0.*.dsc 
rm -f *.upload

rm -f *~
find . -name "*~" -exec rm {} \;

