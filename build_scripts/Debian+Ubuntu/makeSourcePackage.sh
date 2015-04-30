#!/bin/sh

###################### PARAMETERS ####################
version="0.6.0"
svnpath="svn://csoler@svn.code.sf.net/p/retroshare/code/"
workdir=retroshare06-${version}
#bubba3="Y"		# comment out to compile for bubba3
######################################################

echo This script is going to build the debian source package for RetroShare, from the SVN repository.

if test -d "${workdir}" ;  then
    echo Removing the ${workdir} directory...
    rm -rf ${workdir}
fi

# Parse options
svnrev=""
dist=""
# This is the key for "Cyril Soler <csoler@sourceforge.net>"
gpgkey="C737CA98"
while [ ${#} -gt 0 ]; do
    case ${1} in
        "-rev") shift
            svnrev=${1}
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
	dist="precise trusty utopic"
fi

echo "  "Using PGP key id   : ${gpgkey}
echo "  "Using distributions: ${dist}
echo "  "Using svn          : ${rev}

echo Updating SVN...
svn update

if test "${svnrev}" = "" ; then
    echo Attempting to get SVN revision number...
    svnrev=`svn info|awk '/^Revision:/ {print $NF}'`
else
    echo SVN number has been provided. Forcing update.
fi

echo Done.
version="${version}"."${svnrev}"
echo Got version number ${version}. 
echo Please check that the changelog is up to date. 
echo Hit ENTER if this is correct. Otherwise hit Ctrl+C 
read tmp

packages="."

echo SVN number is ${svnrev}
echo Version is ${version}

echo Extracting base archive...

mkdir -p ${workdir}/src
cp -r data   ${workdir}/src/
cp -r debian ${workdir}/debian

echo Checking out latest snapshot...
cd ${workdir}/src
svn co -r${svnrev} ${svnpath}/trunk/ . 
cd -

# VOIP tweak  
cp ${workdir}/src/retroshare-gui/src/gui/chat/PopupChatDialog.ui ${workdir}/src/plugins/VOIP/gui/PopupChatDialog.ui

#   # handling of libssh
#   LIBSSH_VERSION=0.6.4
#   LIBSSH_LOCATION=https://git.libssh.org/projects/libssh.git/snapshot/libssh-${LIBSSH_VERSION}.tar.gz
#   
#   [ -f libssh-${LIBSSH_VERSION}.tar.gz ] || wget --no-check-certificate -O libssh-${LIBSSH_VERSION}.tar.gz $LIBSSH_LOCATION 
#   cd ${workdir}
#   tar zxvf ../libssh-${LIBSSH_VERSION}.tar.gz

# Cloning sqlcipher
# git clone https://github.com/sqlcipher/sqlcipher.git

# cleaning up protobof generated files
rm -f src/retroshare-nogui/src/rpc/proto/gencc/*.pb.h
rm -f src/retroshare-nogui/src/rpc/proto/gencc/*.pb.cc

echo Setting version numbers...

# setup version numbers
sed -e "s%RS_REVISION_NUMBER.*%RS_REVISION_NUMBER   ${svnrev}%" src/libretroshare/src/retroshare/rsversion.in >src/libretroshare/src/retroshare/rsversion.h

# Various cleaning
echo Cleaning...
find . -depth -name ".svn" -a -type d -exec rm -rf {} \;    # remove all svn repositories

echo Calling debuild...
for i in ${dist}; do
    echo copying changelog for ${i}
    sed -e s/XXXXXX/"${svnrev}"/g -e s/YYYYYY/"${i}"/g ../changelog > debian/changelog

    if test "${i}" = "lucid" ; then
        cp ../control.ubuntu_lucid debian/control
    elif test "${i}" = "squeeze" ; then
        cp ../control.squeeze_bubba3 debian/control
    else
        cp ../debian/control debian/control
    fi

    debuild -S -k${gpgkey}
done
cd -

exit 0
