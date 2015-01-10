#!/bin/sh

echo This script will
echo   - remove the directory retroshare-0.5/
echo   - remove existing sources packages in the current directory
echo   - build a new source package from the svn
echo   - rebuild the source package for the karmic i386 arch.
echo 

echo attempting to get svn revision number...
svn=`svn info | grep 'Revision:' | cut -d\  -f2`
echo Revision number is $svn. 

echo Type ^C to abort, or enter to continue
read tmp

#rm -rf ./retroshare-0.5
# ./makeSourcePackage.sh

#for dist in maverick natty; do
for dist in oneiric karmic lucid maverick natty; do
		pbuilder-dist "$dist" build retroshare_0.5.3-0."$svn"~"$dist".dsc
		cp /home/csoler/pbuilder/"$dist"_result/retroshare_0.5.3-0."$svn"~"$dist"_amd64.deb .
		pbuilder-dist "$dist" i386 build retroshare_0.5.3-0."$svn"~"$dist".dsc
		cp /home/csoler/pbuilder/"$dist"-i386_result/retroshare_0.5.3-0."$svn"~"$dist"_i386.deb .
done

