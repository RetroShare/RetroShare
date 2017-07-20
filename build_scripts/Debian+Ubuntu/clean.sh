#!/bin/sh

rm -f ./libssh-0.5.4.tar.gz.*
rm -f ./retroshare_0.6.3-1.*_source.build
rm -f ./retroshare_0.6.3-1.*_source.changes
rm -f ./retroshare_0.6.3-1.*.tar.gz
rm -f ./retroshare_0.6.3-1.*.diff.gz
rm -f ./retroshare_0.6.3-1.*.dsc
rm -f *.upload

rm -f *~
find . -name "*~" -exec rm {} \;

