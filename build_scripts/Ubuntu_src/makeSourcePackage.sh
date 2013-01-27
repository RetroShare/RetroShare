#!/bin/sh

echo This script is going to build the debian source package for RetroShare, from the svn.

svnpath="svn://csoler@svn.code.sf.net/p/retroshare/code/"
workdir=retroshare-0.5
if test -d "$workdir" ;  then
	echo Please remove the $workdir directory first.
	exit
fi

svn update

###################### PARAMETERS ####################
version="0.5.4"
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
cd $workdir/src/libbitdht/
svn co -r$svn $svnpath/trunk/libbitdht/src . 2> /dev/null
cd ../../..
#  
echo Checking out latest snapshot in openpgpsdk...
cd $workdir/src/openpgpsdk/
svn co -r$svn $svnpath/trunk/openpgpsdk/src . 2> /dev/null
cd ../../..
#  
echo Checking out latest snapshot in libretroshare...
cd $workdir/src/libretroshare/
svn co -r$svn $svnpath/trunk/libretroshare/src . 2> /dev/null
cd ../../..
#  
echo Checking out latest snapshot in retroshare-gui...
cd $workdir/src/retroshare-gui/
svn co -r$svn $svnpath/trunk/retroshare-gui/src . 2> /dev/null
cd ../../..
#
echo Checking out latest snapshot in retroshare-nogui...
cd $workdir/src/retroshare-nogui/
svn co -r$svn $svnpath/trunk/retroshare-nogui/src . 2> /dev/null
cd ../../..

# LinksCloud plugin
echo Checking out latest snapshot in LinksCloud plugin
mkdir -p $workdir/src/plugins/LinksCloud
cd $workdir/src/plugins/LinksCloud
svn co -r$svn $svnpath/trunk/plugins/LinksCloud . 2> /dev/null
cd ../../../..

# FeedReader plugin
echo Checking out latest snapshot in FeedReader plugin
mkdir -p $workdir/src/plugins/FeedReader
cd $workdir/src/plugins/FeedReader
svn co -r$svn $svnpath/trunk/plugins/FeedReader . 2> /dev/null
cd ../../../..

# VOIP plugin
echo Checking out latest snapshot in VOIP plugin
mkdir -p $workdir/src/plugins/VOIP
cd $workdir/src/plugins/VOIP
svn co -r$svn $svnpath/trunk/plugins/VOIP . 2> /dev/null
cd ../../../..
cp $workdir/src/retroshare-gui/gui/chat/PopupChatDialog.ui $workdir/src/plugins/VOIP/gui/PopupChatDialog.ui

# common directory in Plugins
cd $workdir/src/plugins
mkdir -p Common
cd Common
svn co -r$svn $svnpath/trunk/plugins/Common . 2> /dev/null
cd ../../../..

# bdboot.txt file
#echo Copying bdboot.txt file at installation place
#cp $workdir/src/libbitdht/bitdht/bdboot.txt

echo Setting version numbers...

# setup version numbers
cat $workdir/src/libretroshare/util/rsversion.h | grep -v SVN_REVISION | grep -v SVN_REVISION_NUMBER > /tmp/toto2342
echo \#define SVN_REVISION \"Revision: "$version"  date : `date`\" >> /tmp/toto2342
echo \#define SVN_REVISION_NUMBER $svn >> /tmp/toto2342
cp /tmp/toto2342 $workdir/src/libretroshare/util/rsversion.h

cat $workdir/src/retroshare-gui/util/rsversion.h | grep -v GUI_REVISION | grep -v SVN_REVISION_NUMBER > /tmp/toto4463
echo \#define GUI_REVISION \"Revision: "$version"  date : `date`\" >> /tmp/toto4463
echo \#define SVN_REVISION_NUMBER $svn >> /tmp/toto4463
cp /tmp/toto4463 $workdir/src/retroshare-gui/util/rsversion.h

# Various cleaning

echo Cleaning...
find $workdir -name ".svn" -exec rm -rf {} \;		# remove all svn repositories

echo Preparing package
#mv $workdir/src/retroshare-gui/RetroShare.pro $workdir/src/retroshare-gui/retroshare-gui.pro

./cleanProFile.sh $workdir/src/libretroshare/libretroshare.pro
./cleanProFile.sh $workdir/src/retroshare-gui/retroshare-gui.pro
./cleanProFile.sh $workdir/src/retroshare-nogui/retroshare-nogui.pro
./cleanProFile_voip.sh $workdir/src/plugins/VOIP/VOIP.pro
./cleanProFile_linkscloud.sh $workdir/src/plugins/LinksCloud/LinksCloud.pro
./cleanProFile_feedreader.sh $workdir/src/plugins/FeedReader/FeedReader.pro

echo "DESTDIR = ../../libretroshare/src/lib/" > /tmp/toto75299
cat $workdir/src/libretroshare/libretroshare.pro /tmp/toto75299 > /tmp/toto752992
cp /tmp/toto752992 $workdir/src/libretroshare/libretroshare.pro

echo "DESTDIR = ../../libbitdht/src/lib/" > /tmp/toto75299
cat $workdir/src/libbitdht/libbitdht.pro /tmp/toto75299 > /tmp/toto752992
cp /tmp/toto752992 $workdir/src/libbitdht/libbitdht.pro

echo "DESTDIR = ../../openpgpsdk/src/lib/" > /tmp/toto75299
cat $workdir/src/openpgpsdk/openpgpsdk.pro /tmp/toto75299 > /tmp/toto752992
cp /tmp/toto752992 $workdir/src/openpgpsdk/openpgpsdk.pro
#cat retroshare-gui-ext.pro >> $workdir/src/retroshare-gui/retroshare-gui.pro 

#echo Building orig directory...
#mkdir $workdir.orig
#cp -r $workdir/src $workdir.orig

# Call debuild to make the source debian package

echo Calling debuild...
cat $workdir/debian/control | sed -e s/XXXXXX/"$version"/g > $workdir/debian/control.tmp
mv -f $workdir/debian/control.tmp $workdir/debian/control

cd $workdir

#for i in sid; do
#for i in natty; do
for i in sid squeeze maverick natty oneiric precise quantal ; do
	echo copying changelog for $i
	cat ../changelog | sed -e s/XXXXXX/"$svn"/g | sed -e s/YYYYYY/"$i"/g > debian/changelog

	# This is the key for "Cyril Soler <csoler@sourceforge.net>"
	debuild -S -kC737CA98
done


