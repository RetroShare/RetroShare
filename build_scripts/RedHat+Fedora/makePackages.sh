#!/bin/bash

###################### PARAMETERS ####################
VERSION="0.6.0"
######################################################
dirs -c

echo "This script is going to build the RedHat family source package for RetroShare, from a clone of the GitHub repository."

# Parse options
while [[ ${#} > 0 ]]
do
    case ${1} in
        "-h") shift
            echo "Package building script for RedHat family distributions"
            echo "Usage:"
            echo "  ${0}"
            exit 1
            ;;
        "*") echo "Unknown option"
            exit 1
            ;;
    esac
done

GITROOT=$(git rev-parse --show-toplevel)
DATE=$(git log --pretty=format:"%ai" | head -1 | cut -d\  -f1 | sed -e s/-//g)
HASH=$(git log --pretty=format:"%H" | head -1 | cut -c1-8)
REV=${DATE}.${HASH}
FULL_VERSION=${VERSION}.${REV}

echo "Using version number: ${VERSION}"
echo "Using revision: ${REV}"
echo "Hit ENTER if this is correct. Otherwise hit Ctrl+C"
read tmp

WORKDIR="retroshare-${FULL_VERSION}"

if [[ -d ${WORKDIR} ]]
then
    echo "Removing the directory ${WORKDIR}..."
    rm -rf ${WORKDIR}
fi
mkdir -p ${WORKDIR}/src

echo "Copying sources into workdir..."
rsync -rc --exclude build_scripts ${GITROOT}/* ${WORKDIR}/src
rsync -rc --copy-links data ${WORKDIR}/src
pushd ${WORKDIR} >/dev/null

# Cloning sqlcipher
echo "Cloning sqlcipher repository..."
mkdir lib
pushd lib >/dev/null
git clone https://github.com/sqlcipher/sqlcipher.git
pushd sqlcipher >/dev/null
git checkout v3.3.1
rm -rf .git
popd >/dev/null
popd >/dev/null

# VOIP tweak  
cp src/retroshare-gui/src/gui/chat/PopupChatDialog.ui src/plugins/VOIP/gui/PopupChatDialog.ui

# cleaning up protobof generated files
rm -f src/retroshare-nogui/src/rpc/proto/gencc/*.pb.h
rm -f src/retroshare-nogui/src/rpc/proto/gencc/*.pb.cc

# setup version numbers
echo "Setting version numbers..."
sed -e "s%RS_REVISION_NUMBER.*%RS_REVISION_NUMBER   0x${HASH}%" src/libretroshare/src/retroshare/rsversion.in >src/libretroshare/src/retroshare/rsversion.h
popd >/dev/null

echo "Creating RPMs..."
[[ -d rpm-build/rpm ]] || mkdir -p rpm-build/rpm
pushd rpm-build/rpm >/dev/null
for DIR in BUILD RPMS SOURCES SPECS SRPMS
do
  [[ -d ${DIR} ]] || mkdir ${DIR}
done
popd >/dev/null
tar -zcf rpm-build/rpm/SOURCES/${WORKDIR}.tar.gz ${WORKDIR}
rpmbuild --define="%rev ${REV}" --define="%_usrsrc $PWD/rpm-build" --define="%_topdir %{_usrsrc}/rpm" -ba retroshare.spec
rm -rf ${WORKDIR}
exit 0
