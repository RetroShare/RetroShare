#!/bin/sh

if test -d "RetroShare" ;  then
	echo Please remove the RetroShare/ directory first.
	exit
fi

packages="."

tar zxvf $packages/BaseRetroShareDirs.tgz

# Ultimately, use the following, but 
cd retroshare-0.5/src/libretroshare/
#tar zxvf ../../../libretroshare.tgz
svn co https://retroshare.svn.sourceforge.net/svnroot/retroshare/trunk/libretroshare/src .
cd ../../..
#  
cd retroshare-0.5/src/retroshare-gui/
svn co https://retroshare.svn.sourceforge.net/svnroot/retroshare/trunk/retroshare-gui/src . 
#tar zxvf ../../../retroshare-gui.tgz
cd ../../..

# Various cleaning

find retroshare-0.5 -name ".svn" -exec rm -rf {} \;		# remove all svn repositories

mv retroshare-0.5/src/retroshare-gui/RetroShare.pro retroshare-0.5/src/retroshare-gui/retroshare-gui.pro

./cleanProFile.sh retroshare-0.5/src/libretroshare/libretroshare.pro
./cleanProFile.sh retroshare-0.5/src/retroshare-gui/retroshare-gui.pro

echo "DESTDIR = ../../libretroshare/src/lib/" > /tmp/toto75299
cat retroshare-0.5/src/libretroshare/libretroshare.pro /tmp/toto75299 > /tmp/toto752992
cp /tmp/toto752992 retroshare-0.5/src/libretroshare/libretroshare.pro

#cat retroshare-gui-ext.pro >> retroshare-0.5/src/retroshare-gui/retroshare-gui.pro 

mkdir retroshare-0.5.orig
cp -r retroshare-0.5/src retroshare-0.5.orig

# Call debuild to make the source debian package

cd retroshare-0.5
debuild -S -kC737CA98

