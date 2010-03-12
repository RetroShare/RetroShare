#!/bin/sh

if test -d "RetroShare" ;  then
	echo Please remove the RetroShare/ directory first.
	exit
fi


###################### PARAMETERS ####################
version="0.5-alpha1"
######################################################

echo attempting to get svn revision number...
svn=`svn info | grep 'Revision:' | cut -d\  -f2`
echo done.
version="$version"."$svn"
echo got version number $version. Is this correct ?
read tmp

packages="."

tar zxvf $packages/BaseRetroShareDirs.tgz

echo Setting up version numbers...
cat retroshare-0.5/debian/control | sed -e s/XXXXXX/"$version"/g | sed -e s/YYYYYY/"$arch"/g | sed -e s/ZZZZZZ/"$packager"/g > retroshare-0.5/debian/control.tmp
mv retroshare-0.5/debian/control.tmp retroshare-0.5/debian/control

echo Getting svn sources
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

echo Cleaning...
find retroshare-0.5 -name ".svn" -exec rm -rf {} \;		# remove all svn repositories

echo Preparing package
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

