#!/bin/sh

###################### PARAMETERS ####################
version="0.4.14-rc1"
arch=`dpkg --print-architecture`
packager="Cyril Soler <csoler@users.sourceforge.net>"

# Directory where to find libretroshare/src/ and retroshare-gui/src
rsdir=/net/cortinaire/media/disc/csoler/RetroShare/retroshare-dev/v0.4.x/

######################################################

echo attempting to get svn revision number...
svn=`svn info | grep 'Revision:' | cut -d\  -f2`

if test "$svn" = "" ; then
	echo Could not read svn version. Please enter it:
	read svn
fi

echo Got svn version $svn
echo done.
version="$version"."$svn"
pkgname=RetroShare_"$version"_ubuntu_"$arch".deb

echo
echo Building retroshare debian package version $version for Ubuntu $arch. 
echo Please check that:
echo "    "- you have sudo access and that root has right to write in this directory and the subdirectories.
echo "    "- you have compiled libretroshare and retroshare-gui in libretroshare/src/ 
echo "                                                        "and retroshare-gui/src/
echo "    "- you have updated version numbers in retroshare-gui/src/util/rsversion.cpp
echo "                                      "and retroshare-gui/src/retroshare.nsi
echo "    "- version and name will be: $pkgname
echo "    "- current '$rsdir' variable is correct:
echo "         "'rsdir='$rsdir

if ! test `whoami` = "root" ; then
	echo Please run this script as root.
	echo
	exit ;
fi

echo
echo Press [ENTER] when ok, else quit with Ctrl+C.
read tmp

echo
echo Unzipping...
tar zxvf retroshare.tgz
echo Changing ownership...
chown -R root.root retroshare 

# setup version numbers and arch in DEBIAN/control

echo Setting up version numbers...
cat retroshare/DEBIAN/control | sed -e s/XXXXXX/"$version"/g | sed -e s/YYYYYY/"$arch"/g | sed -e s/ZZZZZZ/"$packager"/g > retroshare/DEBIAN/control.tmp
mv retroshare/DEBIAN/control.tmp retroshare/DEBIAN/control

# clean
echo Cleaning...
find retroshare -name "*~" -exec \rm -f {} \;

# copy executables at the right place
if ! test -f $rsdir/retroshare-gui/src/RetroShare; then
	echo Can not find executable $rsdir/retroshare-gui/src/RetroShare. Please fix this.
	echo
	exit ;
fi
if ! test -f $rsdir/retroshare-nogui/src/retroshare-nogui; then
	echo Can not find executable $rsdir/retroshare-nogui/src/retroshare-nogui. Please fix this.
	echo
	exit ;
fi
echo Stripping executables...
cp $rsdir/retroshare-gui/src/RetroShare              retroshare/usr/bin/
strip retroshare/usr/bin/RetroShare
cp $rsdir/retroshare-nogui/src/retroshare-nogui retroshare/usr/bin/
strip retroshare/usr/bin/retroshare-nogui

# compute md5 sums
echo Computing/setting md5 sums...
cd retroshare
find usr -type f -exec md5sum {} \; > DEBIAN/md5sums
cd ..
echo Creating package $pkgname 
dpkg-deb -b retroshare $pkgname 

# cleaning
echo Cleaning...
\rm -rf retroshare

echo Done.
