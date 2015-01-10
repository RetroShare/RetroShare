#!/bin/sh

rm BaseRetroShareDirs.tgz 

mv retroshare-0.5/src/data retroshare-0.5/
rm -rf retroshare-0.5/libssh-0.5.2
rm -rf retroshare-0.5/src/*
mv retroshare-0.5/data retroshare-0.5/src/

find retroshare-0.5 -name "*~" -exec \rm {} \;
find retroshare-0.5 -name ".svn" -exec \rm -rf {} \;

tar zcvf BaseRetroShareDirs.tgz retroshare-0.5/

if ! test -f BaseRetroShareDirs.tgz;  then
	echo BaseRetroShareDirs.tgz could not be created
	exit;
fi

rm -rf retroshare-0.5/


