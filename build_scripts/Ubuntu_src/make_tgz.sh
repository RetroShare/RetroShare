#!/bin/sh

rm BaseRetroShareDirs.tgz 

rm -rf retroshare-0.5/libbitdht/
rm -rf retroshare-0.5/libretroshare/

rm -rf retroshare-0.5/src/libbitdht/*
rm -rf retroshare-0.5/src/libbitdht/.svn/
rm -rf retroshare-0.5/src/openpgpsdk/*
rm -rf retroshare-0.5/src/openpgpsdk/.svn/
rm -rf retroshare-0.5/src/libretroshare/*
rm -rf retroshare-0.5/src/libretroshare/.svn/
rm -rf retroshare-0.5/src/retroshare-gui/*
rm -rf retroshare-0.5/src/retroshare-gui/.svn/
rm -rf retroshare-0.5/src/retroshare-nogui/*
rm -rf retroshare-0.5/src/retroshare-nogui/.svn/
rm -rf retroshare-0.5/src/plugins/VOIP/*
rm -rf retroshare-0.5/src/plugins/VOIP/.svn/
rm -rf retroshare-0.5/src/plugins/LinksCloud/*
rm -rf retroshare-0.5/src/plugins/LinksCloud/.svn/

find retroshare-0.5 -name "*~" -exec \rm {} \;

tar zcvf BaseRetroShareDirs.tgz retroshare-0.5/

if ! test -f BaseRetroShareDirs.tgz;  then
	echo BaseRetroShareDirs.tgz could not be created
	exit;
fi

rm -rf retroshare-0.5/


