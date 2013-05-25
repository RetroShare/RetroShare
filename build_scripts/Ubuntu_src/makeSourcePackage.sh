#!/bin/sh

###################### PARAMETERS ####################
version="0.5.4"
svnpath="svn://csoler@svn.code.sf.net/p/retroshare/code/"
workdir=retroshare-$version
######################################################

echo This script is going to build the debian source package for RetroShare, from the svn.

if test -d "$workdir" ;  then
	echo Please remove the $workdir directory first.
	exit
fi

svn update

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

mkdir -p $workdir/src
cp -r debian $workdir/
cp -r data   $workdir/src/

echo Checking out latest snapshot in libbitdht...
cd $workdir/src/
svn co -r$svn $svnpath/trunk/ . 
cd ../..

# VOIP tweak  
cp $workdir/src/retroshare-gui/src/gui/chat/PopupChatDialog.ui $workdir/src/plugins/VOIP/gui/PopupChatDialog.ui

# handling of libssh-0.5.4
wget https://red.libssh.org/attachments/download/41/libssh-0.5.4.tar.gz
cd $workdir
tar zxvf ../libssh-0.5.4.tar.gz
cd ..

# cleaning up protobof generated files
#        \rm -f $workdir/src/rsctrl/src/gencc/*.pb.h
#        \rm -f $workdir/src/rsctrl/src/gencc/*.pb.cpp
\rm -f $workdir/src/retroshare-nogui/src/rpc/proto/gencc/*.pb.h
\rm -f $workdir/src/retroshare-nogui/src/rpc/proto/gencc/*.pb.cc

echo Setting version numbers...

# setup version numbers
cat $workdir/src/libretroshare/src/util/rsversion.h | grep -v SVN_REVISION | grep -v SVN_REVISION_NUMBER > /tmp/toto2342
echo \#define SVN_REVISION \"Revision: "$version"  date : `date`\" >> /tmp/toto2342
echo \#define SVN_REVISION_NUMBER $svn >> /tmp/toto2342
cp /tmp/toto2342 $workdir/src/libretroshare/src/util/rsversion.h

cat $workdir/src/retroshare-gui/src/util/rsversion.h | grep -v GUI_REVISION | grep -v SVN_REVISION_NUMBER > /tmp/toto4463
echo \#define GUI_REVISION \"Revision: "$version"  date : `date`\" >> /tmp/toto4463
echo \#define SVN_REVISION_NUMBER $svn >> /tmp/toto4463
cp /tmp/toto4463 $workdir/src/retroshare-gui/src/util/rsversion.h

# Various cleaning

echo Cleaning...
find $workdir -name ".svn" -exec rm -rf {} \;		# remove all svn repositories

echo Calling debuild...
cat $workdir/debian/control | sed -e s/XXXXXX/"$version"/g > $workdir/debian/control.tmp
mv -f $workdir/debian/control.tmp $workdir/debian/control

cd $workdir

#for i in sid squeeze; do
#for i in precise; do
for i in natty oneiric precise quantal raring; do
	echo copying changelog for $i
	cat ../changelog | sed -e s/XXXXXX/"$svn"/g | sed -e s/YYYYYY/"$i"/g > debian/changelog

	# This is the key for "Cyril Soler <csoler@sourceforge.net>"
	debuild -S -kC737CA98
done
for i in lucid; do
	echo copying changelog for $i
	cat ../changelog | sed -e s/XXXXXX/"$svn"/g | sed -e s/YYYYYY/"$i"/g > debian/changelog
	cp ../control.ubuntu_lucid debian/control

	# This is the key for "Cyril Soler <csoler@sourceforge.net>"
	debuild -S -kC737CA98
done



