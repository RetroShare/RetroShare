#!/bin/sh

echo Getting svn revision...
echo
svn=`svn up | cut -d\  -f3 | cut -d. -f1`

echo This script will
echo   - remove the directory retroshare-0.5/
echo   - remove existing sources packages in the current directory
echo   - build a new source package from the svn
echo   - rebuild the source package for the karmic i386 arch.
echo   - svn number is $svn. Is this correct ?
echo 
echo Type ^C to abort, or type enter to proceed.
read tmp

echo cleaning...
rm -rf ./retroshare-0.5 2> /dev/null
#./clean.sh >& /dev/null
#./makeSourcePackage.sh

sudo PBUILDFOLDER=/var/cache/pbuilder pbuilder-dist lucid i386 build *.dsc
cp /var/cache/pbuilder/lucid-i386_result/retroshare_0.5-1_i386.deb ./RetroShare_0.5."$svn"_lucid_i386.deb

sudo PBUILDFOLDER=/var/cache/pbuilder pbuilder-dist lucid build *.dsc
cp /var/cache/pbuilder/lucid_result/retroshare_0.5-1_amd64.deb ./RetroShare_0.5."$svn"_lucid_amd64.deb

sudo PBUILDFOLDER=/var/cache/pbuilder pbuilder-dist karmic build *.dsc
cp /var/cache/pbuilder/karmic_result/retroshare_0.5-1_amd64.deb ./RetroShare_0.5."$svn"_karmic_amd64.deb

sudo PBUILDFOLDER=/var/cache/pbuilder pbuilder-dist karmic i386 build *.dsc
cp /var/cache/pbuilder/karmic-i386_result/retroshare_0.5-1_i386.deb ./RetroShare_0.5."$svn"_karmic_i386.deb

sudo PBUILDFOLDER=/var/cache/pbuilder pbuilder-dist jaunty build *.dsc
cp /var/cache/pbuilder/jaunty_result/retroshare_0.5-1_amd64.deb ./RetroShare_0.5."$svn"_jaunty_amd64.deb

sudo PBUILDFOLDER=/var/cache/pbuilder pbuilder-dist jaunty i386 build *.dsc
cp /var/cache/pbuilder/jaunty-i386_result/retroshare_0.5-1_i386.deb ./RetroShare_0.5."$svn"_jaunty_i386.deb


mv /media/disc2/csoler/RetroShare_Releases/*.deb /media/disc2/csoler/RetroShare_Releases/Old/
mv *.deb /media/disc2/csoler/RetroShare_Releases/

