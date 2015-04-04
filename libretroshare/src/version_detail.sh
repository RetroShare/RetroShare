#!/bin/bash

#don't exit even if a command fails
set +e

if ( git log -n 1 &> /dev/null); then
	#retrieve git information
	version="git : $(git status | grep branch | cut -c 13-) $(git log -n 1 | grep commit | cut -c 8-)"
fi

if ( git log -n 1 | grep svn &> /dev/null); then
	#retrieve git svn information
	version="${version}  svn : $(git log -n 1 | awk '/svn/ {print $2}' | head -1 | sed 's/.*@//')"
elif ( git log -n 10 | grep svn &> /dev/null); then
	#retrieve git svn information
	version="${version}  svn closest version : $(git log -n 10 | awk '/svn/ {print $2}' | head -1 | sed 's/.*@//')"
fi

if ( svn info &> /dev/null); then
	version=$(svn info | awk '/^Revision:/ {print $NF}')
fi

if [[ ${version} != '' ]]; then
	echo "Writing version to retroshare/rsversion.h : ${version}"
	sed -e "s%RS_REVISION_NUMBER.*%RS_REVISION_NUMBER   ${version}%" retroshare/rsversion.in >retroshare/rsversion.h
fi
echo "script version_detail.sh finished normally"
exit 0
