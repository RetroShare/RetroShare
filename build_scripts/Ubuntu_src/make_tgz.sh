#!/bin/sh

rm BaseRetroShareDirs.tgz 

rm -rf retroshare-0.5/src/libretroshare/*
rm -rf retroshare-0.5/src/libretroshare/.svn/
rm -rf retroshare-0.5/src/retroshare-gui/*
rm -rf retroshare-0.5/src/retroshare-gui/.svn/

find retroshare-0.5 -name "*~" -exec \rm {} \;

tar zcvf BaseRetroShareDirs.tgz retroshare-0.5/

if ! test -f BaseRetroShareDirs.tgz;  then
	echo BaseRetroShareDirs.tgz could not be created
	exit;
fi

rm -rf retroshare-0.5/


