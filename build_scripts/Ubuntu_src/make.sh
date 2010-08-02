#!/bin/sh

echo This script will
echo   - remove the directory retroshare-0.5/
echo   - remove existing sources packages in the current directory
echo   - build a new source package from the svn
echo   - rebuild the source package for the karmic i386 arch.
echo 
echo Type ^C to abort, or \[ENTER\] to continue
read tmp

rm -rf ./retroshare-0.5
./clean.sh
./makeSourcePackage.sh

sudo pbuilder build *.dsc

cp /var/cache/pbuilder/result/retroshare_0.5-1_i386.deb .
