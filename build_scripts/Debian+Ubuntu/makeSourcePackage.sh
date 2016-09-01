#!/bin/sh

###################### PARAMETERS ####################
version="0.6.1"
gitpath="https://github.com/RetroShare/RetroShare.git"
workdir=retroshare06-${version}
#bubba3="Y"		# comment out to compile for bubba3
######################################################

echo This script is going to build the debian source package for RetroShare, from the Git repository.

if test -d "${workdir}" ;  then
    echo Removing the ${workdir} directory...
    rm -rf ${workdir}
fi

# Parse options
rev=""
dist=""
# This is the key for "Cyril Soler <csoler@sourceforge.net>"
gpgkey="0932399B"

while [ ${#} -gt 0 ]; do
    case ${1} in
        "-rev") shift
            rev=${1}
            shift
            ;;
        "-distribution") shift
            dist=${1}
            shift
            ;;
        "-key") shift
            gpgkey=${1}
            shift
            ;;
        "-h") shift
            echo Package building script for debian/ubuntu distributions
            echo Usage:
            echo "  "${0} '-key [PGP key id] -rev [svn revision number]  -distribution [distrib name list with quotes, in (wheezy, sid, precise, saucy, etc)]'
            exit 1
            ;;
        "*") echo "Unknown option"
            exit 1
            ;;
    esac
done

if test "${dist}" = "" ; then
	dist="precise trusty vivid wily xenial wheezy squeeze jessie stretch"
fi

echo Attempting to get revision number...
ccount=`git rev-list --count --all`
ccount=`expr $ccount + 8613 - 8267`

date=`git log --pretty=format:"%ai" | head -1 | cut -d\  -f1 | sed -e s/-//g`
time=`git log --pretty=format:"%aD" | head -1 | cut -d\  -f5 | sed -e s/://g`
hhsh=`git log --pretty=format:"%H" | head -1 | cut -c1-8`

rev=${date}.${hhsh}

echo "  "Using PGP key id   : ${gpgkey}
echo "  "Using distributions: ${dist}
echo "  "Commit count       : ${ccount}
echo "  "Date               : ${date}
echo "  "Time               : ${time}
echo "  "Hash               : ${hhsh}
echo "  "Using revision     : ${rev}

echo Done.
version="${version}"."${rev}"
echo Got version number ${version}. 
echo Please check that the changelog is up to date. 
echo Hit ENTER if this is correct. Otherwise hit Ctrl+C 
read tmp

echo Extracting base archive...

mkdir -p ${workdir}/src
echo Checking out latest snapshot...
cd ${workdir}/src
git clone --depth 1 https://github.com/RetroShare/RetroShare.git .
cd -

if ! test -d ${workdir}/src/libretroshare/; then
	echo Git clone failed. 
	exit
fi

#cp -r data   ${workdir}/src/
cp -r debian ${workdir}/debian

#svn co -r${rev} ${svnpath}/trunk/ . 

# VOIP tweak  
cp ${workdir}/src/retroshare-gui/src/gui/chat/PopupChatDialog.ui ${workdir}/src/plugins/VOIP/gui/PopupChatDialog.ui

#   # handling of libssh
#   LIBSSH_VERSION=0.6.4
#   LIBSSH_LOCATION=https://git.libssh.org/projects/libssh.git/snapshot/libssh-${LIBSSH_VERSION}.tar.gz
#   
#   [ -f libssh-${LIBSSH_VERSION}.tar.gz ] || wget --no-check-certificate -O libssh-${LIBSSH_VERSION}.tar.gz $LIBSSH_LOCATION 
#   tar zxvf ../libssh-${LIBSSH_VERSION}.tar.gz

# Cloning sqlcipher
# git clone https://github.com/sqlcipher/sqlcipher.git

# cleaning up protobof generated files
# rm -f src/retroshare-nogui/src/rpc/proto/gencc/*.pb.h
# rm -f src/retroshare-nogui/src/rpc/proto/gencc/*.pb.cc

cd ${workdir}
echo Setting version numbers...

# setup version numbers
sed -e "s%RS_REVISION_NUMBER.*%RS_REVISION_NUMBER   0x${hhsh}%"  src/libretroshare/src/retroshare/rsversion.in > src/libretroshare/src/retroshare/rsversion.h

# Various cleaning
echo Cleaning...

\rm -rf src/.git
#find . -depth -name ".svn" -a -type d -exec rm -rf {} \;    # remove all svn repositories

echo Calling debuild...
for i in ${dist}; do
    echo copying changelog for ${i}
    sed -e s/XXXXXX/"${rev}"/g -e s/YYYYYY/"${i}"/g ../changelog > debian/changelog

    if test "${i}" = "lucid" ; then
        cp ../control.ubuntu_lucid debian/control
    elif test "${i}" = "squeeze" ; then
        cp ../control.squeeze_bubba3 debian/control
    elif test "${i}" = "precise" ; then
        cp ../control.precise debian/control
    elif test "${i}" = "xenial" ; then
        cp ../control.xenial debian/control
    elif test "${i}" = "stretch" ; then
        cp ../control.${i} debian/control
        cp ../rules.${i} debian/rules
    else
        cp ../debian/control debian/control
    fi

    debuild -S -k${gpgkey}
done
cd -

exit 0
