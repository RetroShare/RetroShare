#!/bin/bash

if ( git log -n 1 &> /dev/null); then
	#retrieve git information
	version="git : $(git status | grep branch | cut -c 6-) $(git log -n 1 | grep commit)"
fi

if ( git log -n 1 | grep svn &> /dev/null); then
	#retrieve git svn information
	version="$version  svn : $(git log -n 1 | grep svn | awk '{print $2}' | head -1 | sed 's/.*@//')"
elif ( git log -n 10 | grep svn &> /dev/null); then
	#retrieve git svn information
	version="$version  svn closest version : $(git log -n 10 | grep svn | awk '{print $2}' | head -1 | sed 's/.*@//')"
fi

if ( svn info &> /dev/null); then
	version=$(svn info | head -n 5 | tail -1)
fi
if [[ $version != '' ]]; then
	version="$version  date : $(date +'%T %m.%d.%y')"
	echo "Writing version to util/rsversion.h : $version "
	sed -i "s/LIB_VERSION .*/LIB_VERSION \"$version\"/g" util/rsversion.h
fi
