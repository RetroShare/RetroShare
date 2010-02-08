#!/bin/bash

#don't exit even if a command fails
set +e

if ( svn info &> /dev/null); then
	version=$(svn info | head -n 5 | tail -1)
fi
if [[ $version != '' ]]; then
	version="$version  date : $(date +'%T %m.%d.%y')"
	echo "Writing version to util/rsversion.h : $version "
	sed -i "s/SVN_REVISION .*/SVN_REVISION \"$version\"/g" util/rsversion.h
fi
echo "script version_detail.sh finished normally"
exit 0
