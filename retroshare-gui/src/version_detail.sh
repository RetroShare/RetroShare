#!/bin/bash
#check if we're on *nix system
#write the version.html file

#don't exit even if a command fails
set +e

pushd $(dirname "$0")

OLDLANG=${LANG}

export LANG=C

SCRIPT_PATH=$(dirname "`readlink -f "${0}"`")

if (ls &> /dev/null); then
	echo "Retroshare Gui version : " > ${SCRIPT_PATH}/gui/help/version.html
	if ( /usr/bin/git log -n 1 &> /dev/null); then
		#retrieve git information
		echo "Git version : $(git status | grep branch | head -n 1 | cut -c 4-) $(git log -n 1 | grep commit)" >> ${SCRIPT_PATH}/gui/help/version.html
	fi
	if ( /usr/bin/git log -n 1 | grep svn &> /dev/null); then
		#retrieve git svn information
		echo "Svn version : $(git log -n 1 | awk '/svn/ {print $2}' | head -1)" >> ${SCRIPT_PATH}/gui/help/version.html
	elif ( /usr/bin/git log -n 10 | grep svn &> /dev/null); then
		#retrieve git svn information
		echo "Svn closest version : $(git log -n 10 | awk '/svn/ {print $2}' | head -1)" >> ${SCRIPT_PATH}/gui/help/version.html
	fi

	if ( /usr/bin/svn info &> /dev/null); then
		echo "Svn version : $(svn info | awk '/^Revision:/ {print $NF}')" >> ${SCRIPT_PATH}/gui/help/version.html
	fi
	date >> ${SCRIPT_PATH}/gui/help/version.html
	echo "" >> ${SCRIPT_PATH}/gui/help/version.html
	echo "" >> ${SCRIPT_PATH}/gui/help/version.html
fi

export LANG=${OLDLANG}

popd

echo "version_detail.sh scripts finished"
exit 0
