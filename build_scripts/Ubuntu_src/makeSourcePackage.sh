#!/bin/sh

echo This script is going to build the debian source package for RetroShare, from the svn.

if test -d "RetroShare" ;  then
	echo Please remove the RetroShare/ directory first.
	exit
fi

svn update

###################### PARAMETERS ####################
version="0.5.1"
######################################################

if test "$1" = "" ; then
	echo attempting to get svn revision number...
	svn=`svn info | grep 'Revision:' | cut -d\  -f2`
else
	echo svn number has been provided. Forcing update.
	svn="$1"
fi

echo done.
version="$version"."$svn"
echo got version number $version. 
echo Please check that the changelog is up to date. 
echo Hit ENTER is this is this correct. Otherwise hit Ctrl+C 
read tmp

packages="."

echo SVN number is $svn
echo version is $version

echo Extracting base archive...
tar zxvf $packages/BaseRetroShareDirs.tgz 2> /dev/null

echo Checking out latest snapshot in libbitdht...
cd retroshare-0.5/src/libbitdht/
svn co -r$svn https://retroshare.svn.sourceforge.net/svnroot/retroshare/trunk/libbitdht/src . 2> /dev/null
cd ../../..
#  
echo Checking out latest snapshot in libretroshare...
cd retroshare-0.5/src/libretroshare/
svn co -r$svn https://retroshare.svn.sourceforge.net/svnroot/retroshare/trunk/libretroshare/src . 2> /dev/null
cd ../../..
#  
echo Checking out latest snapshot in retroshare-gui...
cd retroshare-0.5/src/retroshare-gui/
svn co -r$svn https://retroshare.svn.sourceforge.net/svnroot/retroshare/trunk/retroshare-gui/src . 2> /dev/null
cd ../../..
#
echo Checking out latest snapshot in retroshare-nogui...
cd retroshare-0.5/src/retroshare-nogui/
svn co -r$svn https://retroshare.svn.sourceforge.net/svnroot/retroshare/trunk/retroshare-nogui/src . 2> /dev/null
cd ../../..

echo Copying bdboot.txt file at installation place
cp retroshare-0.5/src/libbitdht/bitdht/bdboot.txt
echo Setting version numbers...

# setup version numbers
cat retroshare-0.5/src/libretroshare/util/rsversion.h | grep -v SVN_REVISION > /tmp/toto2342
echo \#define SVN_REVISION \"Revision: "$version"  date : `date`\" >> /tmp/toto2342
cp /tmp/toto2342 retroshare-0.5/src/libretroshare/util/rsversion.h

cat retroshare-0.5/src/retroshare-gui/util/rsversion.h | grep -v GUI_REVISION > /tmp/toto4463
echo \#define GUI_REVISION \"Revision: "$version"  date : `date`\" >> /tmp/toto4463
cp /tmp/toto4463 retroshare-0.5/src/retroshare-gui/util/rsversion.h

# Various cleaning

echo Cleaning...
find retroshare-0.5 -name ".svn" -exec rm -rf {} \;		# remove all svn repositories

echo Preparing package
mv retroshare-0.5/src/retroshare-gui/RetroShare.pro retroshare-0.5/src/retroshare-gui/retroshare-gui.pro

./cleanProFile.sh retroshare-0.5/src/libretroshare/libretroshare.pro
./cleanProFile.sh retroshare-0.5/src/retroshare-gui/retroshare-gui.pro
./cleanProFile.sh retroshare-0.5/src/retroshare-nogui/retroshare-nogui.pro

echo "DESTDIR = ../../libretroshare/src/lib/" > /tmp/toto75299
cat retroshare-0.5/src/libretroshare/libretroshare.pro /tmp/toto75299 > /tmp/toto752992
cp /tmp/toto752992 retroshare-0.5/src/libretroshare/libretroshare.pro

echo "DESTDIR = ../../libbitdht/src/lib/" > /tmp/toto75299
cat retroshare-0.5/src/libbitdht/libbitdht.pro /tmp/toto75299 > /tmp/toto752992
cp /tmp/toto752992 retroshare-0.5/src/libbitdht/libbitdht.pro

#cat retroshare-gui-ext.pro >> retroshare-0.5/src/retroshare-gui/retroshare-gui.pro 

#echo Building orig directory...
#mkdir retroshare-0.5.orig
#cp -r retroshare-0.5/src retroshare-0.5.orig

# Call debuild to make the source debian package

echo Calling debuild...
cat retroshare-0.5/debian/control | sed -e s/XXXXXX/"$version"/g > retroshare-0.5/debian/control.tmp
mv -f retroshare-0.5/debian/control.tmp retroshare-0.5/debian/control

cd retroshare-0.5

for i in lucid karmic jaunty maverick natty; do
	echo copying changelog for $i
	cat ../changelog | sed -e s/XXXXXX/"$svn"/g | sed -e s/YYYYYY/"$i"/g > debian/changelog

	# This is the key for "Cyril Soler <csoler@sourceforge.net>"
	debuild -S -kC737CA98
done


