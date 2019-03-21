#!/bin/bash

NAME=sqlcipher
# this is a huge hunk of stuff, so reuse the local repo if possible
if [ -d ${NAME}/.git ]; then
	cd ${NAME}
	git pull
	cd ..
else
	set -e
	git clone https://github.com/sqlcipher/sqlcipher.git
	set +e
fi

TOPDIR=$(pwd)
cd ${NAME}
TAG=$(git tag -l | tail -n 1)
LINE=$(git show --format=format:"%h %ai"|head -n 1)
set -- $LINE
REV=$1
DATE=$2
VER=${DATE//-/.}
set -e
git archive --prefix=${NAME}-${TAG#v}/ -o $TOPDIR/${NAME}-${TAG#v}.tar ${TAG}
cd $TOPDIR
bzip2 -9 ${NAME}-${TAG#v}.tar
sed -i "s/^Version:.*/Version:        ${TAG#v}/" ${NAME}.spec
osc vc -m "Update to $REV ($DATE)"
