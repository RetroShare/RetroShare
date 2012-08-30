#!/bin/sh

echo This script is going to build the debian source package for RetroShare LinksCloud plugin, from the svn.
nosvn=true
workdir=retroshare-linkscloud-plugin-0.5.3

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
echo Checking out latest snapshot in LinksCloud plugin
mkdir -p $workdir/src/plugins/LinksCloud
if test "$nosvn" = "true"; then
	cd $workdir/src/plugins/
	cp -r ../../../svn_image/plugins/LinksCloud .
	cp -r ../../../svn_image/plugins/Common .
else
	cd $workdir/src/plugins/LinksCloud
	svn co -r$svn https://retroshare.svn.sourceforge.net/svnroot/retroshare/trunk/plugins/LinksCloud . 2> /dev/null
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

cp linkscloud-plugin/src.pro $workdir/src/src.pro
cp linkscloud-plugin/plugins.pro $workdir/src/plugins/plugins.pro
./linkscloud-plugin/cleanProFile.sh $workdir/src/plugins/LinksCloud/LinksCloud.pro

echo Calling debuild...
cp linkscloud-plugin/debian_rules $workdir/debian/rules
cat linkscloud-plugin/debian_control | sed -e s/XXXXXX/"$version"/g > $workdir/debian/control.tmp
mv -f $workdir/debian/control.tmp $workdir/debian/control

cd $workdir

for i in natty; do
#for i in precise squeeze oneiric karmic lucid maverick natty; do
	echo copying changelog for $i
	cat ../linkscloud-plugin/changelog | sed -e s/XXXXXX/"$svn"/g | sed -e s/YYYYYY/"$i"/g > debian/changelog

	# This is the key for "Cyril Soler <csoler@sourceforge.net>"
	debuild -S -kC737CA98
done


