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

svn=4153
rm -rf ./retroshare-0.5
# ./makeSourcePackage.sh

for dist in karmic lucid maverick natty; do
		sudo PBUILDFOLDER=/var/cache/pbuilder pbuilder-dist "$dist" build retroshare_0.5.2-0."$svn"~"$dist".dsc
		cp /var/cache/pbuilder/"$dist"_result/retroshare_0.5.2-0."$svn"~"$dist"_amd64.deb .
		sudo PBUILDFOLDER=/var/cache/pbuilder pbuilder-dist "$dist" i386 build retroshare_0.5.2-0."$svn"~"$dist".dsc
		cp /var/cache/pbuilder/"$dist"-i386_result/retroshare_0.5.2-0."$svn"~"$dist"_i386.deb .
done

