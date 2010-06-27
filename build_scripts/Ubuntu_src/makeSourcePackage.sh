#!/bin/sh

if test -d "RetroShare" ;  then
	echo Please remove the RetroShare/ directory first.
	exit
fi

svnnumber=`svn up | cut -d\  -f3 | cut -d. -f1`
version=0.5.0."$svnnumber"
packages="."

echo SVN number is $svnnumber
echo version is $version

tar zxvf $packages/BaseRetroShareDirs.tgz

# Ultimately, use the following, but 
cd retroshare-0.5/src/libretroshare/
#tar zxvf ../../../libretroshare.tgz
svn co https://retroshare.svn.sourceforge.net/svnroot/retroshare/branches/v0.5.0/libretroshare/src .
cd ../../..
#  
cd retroshare-0.5/src/retroshare-gui/
svn co https://retroshare.svn.sourceforge.net/svnroot/retroshare/branches/v0.5.0/retroshare-gui/src . 
#tar zxvf ../../../retroshare-gui.tgz
cd ../../..
#
cd retroshare-0.5/src/retroshare-nogui/
svn co https://retroshare.svn.sourceforge.net/svnroot/retroshare/branches/v0.5.0/retroshare-nogui/src . 
#tar zxvf ../../../retroshare-gui.tgz
cd ../../..


# setup version numbers
cat retroshare-0.5/src/libretroshare/util/rsversion.h | grep -v SVN_REVISION > /tmp/toto2342
echo \#define SVN_REVISION \"Revision: "$svnnumber"  date : `date`\" >> /tmp/toto2342
cp /tmp/toto2342 retroshare-0.5/src/libretroshare/util/rsversion.h

cat retroshare-0.5/src/retroshare-gui/util/rsversion.h | grep -v GUI_REVISION > /tmp/toto4463
echo \#define GUI_REVISION \"Revision: "$svnnumber"  date : `date`\" >> /tmp/toto4463
cp /tmp/toto4463 retroshare-0.5/src/retroshare-gui/util/rsversion.h

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

cat retroshare-0.5/debian/control | sed -e s/XXXXXX/"$version"/g > retroshare-0.5/debian/control.tmp
mv -f retroshare-0.5/debian/control.tmp retroshare-0.5/debian/control

cd retroshare-0.5
debuild -S -kC737CA98

