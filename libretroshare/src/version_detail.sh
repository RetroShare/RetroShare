#!/bin/bash

#don't exit even if a command fails
set +e


if ( git log -n 1 &> /dev/null); then
	#retrieve git information
	version="git : $(git status | grep branch | cut -c 13-) $(git log -n 1 | grep commit | cut -c 8-)"
fi

if ( git log -n 1 | grep svn &> /dev/null); then
	#retrieve git svn information
	version="$version  svn : $(git log -n 1 | grep svn | awk '{print $2}' | head -1 | sed 's/.*@//')"
elif ( git log -n 10 | grep svn &> /dev/null); then
	#retrieve git svn information
	version="$version  svn closest version : $(git log -n 10 | grep svn | awk '{print $2}' | head -1 | sed 's/.*@//')"
fi

if ( svn info &> /dev/null); then
	version=$(svn info | grep '^Revision:')
fi

if [[ $version != '' ]]; then
	version_number=`echo $version | cut -d: -f2`
	version="$version  date : $(date +'%T %m.%d.%y')"
	echo "Writing version to util/rsversion.h : $version "
	sed -i "s/SVN_REVISION .*/SVN_REVISION \"$version\"/g" util/rsversion.h
	sed -i "s/SVN_REVISION_NUMBER .*/SVN_REVISION_NUMBER $version_number/g" util/rsversion.h
fi
echo "script version_detail.sh finished normally"
exit 0
