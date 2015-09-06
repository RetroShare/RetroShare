#!/bin/bash

###################### PARAMETERS ####################
VERSION="0.6.0"
######################################################
dirs -c

echo "This script is going to build the debian source package for RetroShare, from a clone of the GitHub repository."
echo

# Parse options
REV=""
DISTS=""
# This is the key for "Cyril Soler <csoler@sourceforge.net>"
GPGKEY="0932399B"

while [ ${#} -gt 0 ]; do
    case ${1} in
        "-distribution") shift
            DISTS=${1}
            shift
            ;;
        "-key") shift
            GPGKEY=${1}
            shift
            ;;
        "-h") shift
            echo "Package building script for debian/ubuntu distributions"
            echo "Usage:"
            echo "  ${0} '-key [PGP key id] -rev [svn revision number]  -distribution [distrib name list with quotes, in (wheezy, sid, precise, saucy, etc)]'"
            exit 1
            ;;
        "*") echo "Unknown option"
            exit 1
            ;;
    esac
done

if test "${DISTS}" = "" ; then
	DISTS="precise trusty vivid"
fi

echo Attempting to get revision number...
CCOUNT=`git rev-list --count --all`
CCOUNT=`expr ${CCOUNT} + 8613 - 8267`

DATE=`git log --pretty=format:"%ai" | head -1 | cut -d\  -f1 | sed -e s/-//g`
TIME=`git log --pretty=format:"%aD" | head -1 | cut -d\  -f5 | sed -e s/://g`
HASH=`git log --pretty=format:"%H" | head -1 | cut -c1-8`

GITROOT=`git rev-parse --show-toplevel`
REV=${DATE}.${HASH}
FULL_VERSION="${VERSION}"."${REV}"

echo "Using PGP key id    : ${GPGKEY}"
echo "Using distributions : ${DISTS}"
echo "Commit count        : ${CCOUNT}"
echo "Date                : ${DATE}"
echo "Time                : ${TIME}"
echo "Hash                : ${HASH}"
echo "Using revision      : ${REV}"
echo "Using version number: ${VERSION}"
echo
echo "Hit ENTER if this is correct. Otherwise hit Ctrl+C"
read tmp

WORKDIR="retroshare06-${FULL_VERSION}"

if [[ -d ${WORKDIR} ]]
then
    echo "Removing the directory ${WORKDIR}..."
    rm -rf ${WORKDIR}
fi
mkdir -p ${WORKDIR}/src

echo "Copying sources into workdir..."
rsync -rc --exclude build_scripts ${GITROOT}/* ${WORKDIR}/src
cp -r debian ${WORKDIR}/debian
pushd ${WORKDIR} >/dev/null

# VOIP tweak  
cp src/retroshare-gui/src/gui/chat/PopupChatDialog.ui src/plugins/VOIP/gui/PopupChatDialog.ui

echo "Setting version numbers..."
sed -e "s%RS_REVISION_NUMBER.*%RS_REVISION_NUMBER   0x${HASH}%"  src/libretroshare/src/retroshare/rsversion.in > src/libretroshare/src/retroshare/rsversion.h

echo "Calling debuild..."
for DIST in ${DISTS}; do
    echo "opying changelog for ${DIST}..."
    sed -e s/XXXXXX/"${REV}"/g -e s/YYYYYY/"${DIST}"/g ../changelog > debian/changelog

    [ -d sqlcipher ] && rm -rf sqlcipher

    if test "${DIST}" = "lucid" ; then
        cp ../control.ubuntu_${DIST} debian/control
    elif test "${DIST}" = "squeeze" ; then
        cp ../control.${DIST}_bubba3 debian/control
    elif test "${DIST}" = "precise" ; then
        cp ../control.${DIST} debian/control
    elif test "${DIST}" = "wheezy" -o "${DIST}" = "jessie" ; then
        # Cloning sqlcipher
        git clone https://github.com/sqlcipher/sqlcipher.git
        pushd sqlcipher >/dev/null
        git checkout v3.3.1
        rm -rf .git
        popd >/dev/null

        cp ../control.${DIST} debian/control
        cp ../rules.${DIST} debian/rules
    else
        cp ../debian/control debian/control
    fi

    debuild -S -k${GPGKEY}
done
popd >/dev/null

exit 0
