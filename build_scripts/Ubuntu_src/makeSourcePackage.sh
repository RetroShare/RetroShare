#!/bin/sh

if test -d "RetroShare" ;  then
	echo Please remove the RetroShare/ directory first.
	exit
fi

packages="."

tar zxvf $packages/BaseRetroShareDirs.tgz

cd retroshare-0.5/src/libretroshare/
svn co https://retroshare.svn.sourceforge.net/svnroot/retroshare/trunk/libretroshare/src .
cd ../../..

cd retroshare-0.5/src/retroshare-gui/
svn co https://retroshare.svn.sourceforge.net/svnroot/retroshare/trunk/retroshare-gui/src . 
cd ../../..

cd retroshare-0.5
debuild -S

