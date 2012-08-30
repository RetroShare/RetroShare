#!/bin/sh

echo This script is going to build the debian source package for RetroShare VOIP plugin, from the svn.
nosvn=true
workdir=retroshare-voip-plugin-0.5.3

if test -d "$workdir" ;  then
	echo Please remove the $workdir directory first.
	exit
fi

if test -d "retroshare-0.5" ;  then
	echo Please remove the retroshare-0.5/ directory first.
	exit
fi

svn update

###################### PARAMETERS ####################
version="0.5.3"
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
mv retroshare-0.5 $workdir

#  echo Checking out latest snapshot in libbitdht...
#  cd $workdir/src/libbitdht/
#  svn co -r$svn https://retroshare.svn.sourceforge.net/svnroot/retroshare/trunk/libbitdht/src . 2> /dev/null
#  cd ../../..
#  #  
#  echo Checking out latest snapshot in openpgpsdk...
#  cd $workdir/src/openpgpsdk/
#  svn co -r$svn https://retroshare.svn.sourceforge.net/svnroot/retroshare/trunk/openpgpsdk/src . 2> /dev/null
#  cd ../../..
#  
echo Checking out latest snapshot in libretroshare...
cd $workdir/src/libretroshare/
if test "$nosvn" = "true"; then
	cp -r ../../../svn_image/libretroshare/src/* .
else
	svn co -r$svn https://retroshare.svn.sourceforge.net/svnroot/retroshare/trunk/libretroshare/src . 2> /dev/null
fi
cd ../../..
#  
echo Checking out latest snapshot in retroshare-gui...
cd $workdir/src/retroshare-gui/
if test "$nosvn" = "true"; then
	cp -r ../../../svn_image/retroshare-gui/src/* .
else
	svn co -r$svn https://retroshare.svn.sourceforge.net/svnroot/retroshare/trunk/retroshare-gui/src . 2> /dev/null
fi
cd ../../..
#
echo Checking out latest snapshot in VOIP plugin
mkdir -p $workdir/src/plugins/VOIP
if test "$nosvn" = "true"; then
	cd $workdir/src/plugins/
	cp -r ../../../svn_image/plugins/VOIP .
	cp -r ../../../svn_image/plugins/Common .
else
	cd $workdir/src/plugins/VOIP
	svn co -r$svn https://retroshare.svn.sourceforge.net/svnroot/retroshare/trunk/plugins/VOIP . 2> /dev/null
	cd ..
	mkdir -p Common
	cd Common
	svn co -r$svn https://retroshare.svn.sourceforge.net/svnroot/retroshare/trunk/plugins/Common . 2> /dev/null
fi
cd ../../..
#
#  echo Checking out latest snapshot in retroshare-nogui...
#  cd $workdir/src/retroshare-nogui/
#  svn co -r$svn https://retroshare.svn.sourceforge.net/svnroot/retroshare/trunk/retroshare-nogui/src . 2> /dev/null
#  cd ../../..

#  echo Copying bdboot.txt file at installation place
#  cp $workdir/src/libbitdht/bitdht/bdboot.txt

echo Setting version numbers...
#  
# setup version numbers
cat $workdir/src/libretroshare/util/rsversion.h | grep -v SVN_REVISION > /tmp/toto2342
echo \#define SVN_REVISION \"Revision: "$version"  date : `date`\" >> /tmp/toto2342
cp /tmp/toto2342 $workdir/src/libretroshare/util/rsversion.h

cat $workdir/src/retroshare-gui/util/rsversion.h | grep -v GUI_REVISION > /tmp/toto4463
echo \#define GUI_REVISION \"Revision: "$version"  date : `date`\" >> /tmp/toto4463
cp /tmp/toto4463 $workdir/src/retroshare-gui/util/rsversion.h

# Various cleaning

echo Cleaning...
find $workdir -name ".svn" -exec rm -rf {} \;		# remove all svn repositories

if test "$nosvn" = "true"; then
	find $workdir -name ".o" -exec rm -rf {} \;		# remove all svn repositories
	find $workdir -name ".a" -exec rm -rf {} \;		# remove all svn repositories
fi

echo Preparing package
#mv $workdir/src/retroshare-gui/RetroShare.pro $workdir/src/retroshare-gui/retroshare-gui.pro

#./cleanProFile.sh $workdir/src/libretroshare/libretroshare.pro
#./cleanProFile.sh $workdir/src/retroshare-gui/retroshare-gui.pro

cp voip-plugin/src.pro $workdir/src/src.pro
cp voip-plugin/plugins.pro $workdir/src/plugins/plugins.pro
cp $workdir/src/retroshare-gui/gui/chat/PopupChatDialog.ui $workdir/src/plugins/VOIP/gui/PopupChatDialog.ui
./voip-plugin/cleanProFile.sh $workdir/src/plugins/VOIP/VOIP.pro

#echo "DESTDIR = ../../voip-plugin/src/lib/" > /tmp/toto75299
#cat $workdir/src/libretroshare/libretroshare.pro /tmp/toto75299 > /tmp/toto752992
#cp /tmp/toto752992 $workdir/src/libretroshare/libretroshare.pro

#echo "DESTDIR = ../../libbitdht/src/lib/" > /tmp/toto75299
#cat $workdir/src/libbitdht/libbitdht.pro /tmp/toto75299 > /tmp/toto752992
#cp /tmp/toto752992 $workdir/src/libbitdht/libbitdht.pro
#
#echo "DESTDIR = ../../openpgpsdk/src/lib/" > /tmp/toto75299
#cat $workdir/src/openpgpsdk/openpgpsdk.pro /tmp/toto75299 > /tmp/toto752992
#cp /tmp/toto752992 $workdir/src/openpgpsdk/openpgpsdk.pro
#cat retroshare-gui-ext.pro >> $workdir/src/retroshare-gui/retroshare-gui.pro 

#echo Building orig directory...
#mkdir $workdir.orig
#cp -r $workdir/src $workdir.orig

# Call debuild to make the source debian package

echo Calling debuild...
cp voip-plugin/debian_rules $workdir/debian/rules
cat voip-plugin/debian_control | sed -e s/XXXXXX/"$version"/g > $workdir/debian/control.tmp
mv -f $workdir/debian/control.tmp $workdir/debian/control

cd $workdir

for i in natty; do
#for i in precise squeeze oneiric karmic lucid maverick natty; do
	echo copying changelog for $i
	cat ../voip-plugin/changelog | sed -e s/XXXXXX/"$svn"/g | sed -e s/YYYYYY/"$i"/g > debian/changelog

	# This is the key for "Cyril Soler <csoler@sourceforge.net>"
	debuild -S -kC737CA98
done


