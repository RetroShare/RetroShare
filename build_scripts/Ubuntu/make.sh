#!/bin/sh

###################### PARAMETERS ####################
version="0.4.11d"
arch=`dpkg --print-architecture`
packager="Cyril Soler <csoler@users.sourceforge.net>"
######################################################

echo attempting to get svn revision number...
svn=`svn info | grep 'Revision:' | cut -d\  -f2`
echo done.
version="$version"."$svn"
echo
echo Building retroshare debian package version $version for Ubuntu $arch. 
echo Please check that:
echo "    "- you have sudo access and that root has right to write in this directory and the subdirectories.
echo "    "- you have compiled libretroshare and retroshare-gui in ../../libretroshare/src/ 
echo "                                                        "and ../../retroshare-gui/src/
echo "    "- you have updated version numbers in ../../retroshare-gui/src/util/rsversion.cpp
echo "                                      "and ../../retroshare-gui/src/retroshare.nsi
echo "    "- version and name will be: RetroShare_"$version"_ubuntu_hardy_"$arch".deb

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
if ! test -f ../../retroshare-gui/src/RetroShare; then
	echo Can not find executable ../../retroshare-gui/src/RetroShare. Please fix this.
	echo
	exit ;
fi
if ! test -f ../../libretroshare/src/rsiface/retroshare-nogui; then
	echo Can not find executable ../../libretroshare/src/rsiface/retroshare-nogui. Please fix this.
	echo
	exit ;
fi
echo Stripping executables...
cp ../../retroshare-gui/src/RetroShare              retroshare/usr/bin/
strip retroshare/usr/bin/RetroShare
cp ../../libretroshare/src/rsiface/retroshare-nogui retroshare/usr/bin/
strip retroshare/usr/bin/retroshare-nogui

# compute md5 sums
echo Computing/setting md5 sums...
cd retroshare
find usr -type f -exec md5sum {} \; > DEBIAN/md5sums
cd ..
echo Creating package RetroShare_"$version"_ubuntu_hardy_"$arch".deb

dpkg-deb -b retroshare RetroShare_"$version"_ubuntu_hardy_"$arch".deb

# cleaning
echo Cleaning...
\rm -rf retroshare

echo Done.
