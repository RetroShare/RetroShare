#!/bin/sh

###################### PARAMETERS ####################
version="0.6.3"
gitpath="https://github.com/RetroShare/RetroShare.git"
workdir=retroshare-${version}
branch="master"
#branch="v0.6.3-official_release"
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

date=`git log --pretty=format:"%ai" | head -1 | cut -d\  -f1 | sed -e s/-//g`
time=`git log --pretty=format:"%aD" | head -1 | cut -d\  -f5 | sed -e s/://g`
hhsh=`git log --pretty=format:"%H" | head -1 | cut -c1-8`

rev=${date}.${hhsh}

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
	dist="precise trusty xenial zesty artful"
fi

echo Attempting to get revision number...
ccount=`git rev-list --count --all`
ccount=`expr $ccount + 8613 - 8267`

echo "  "Using PGP key id   : ${gpgkey}
echo "  "Using distributions: ${dist}
echo "  "Commit count       : ${ccount}
echo "  "Date               : ${date}
echo "  "Time               : ${time}
echo "  "Hash               : ${hhsh}
echo "  "Using branch       : ${branch}
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
git clone --depth 1 https://github.com/RetroShare/RetroShare.git --single-branch --branch $branch .

#  if ! test "$hhsh" = "" ; then
#  	echo Checking out revision $hhsh
#  	git checkout $hhsh
#  fi

cd -

if ! test -d ${workdir}/src/libretroshare/; then
	echo Git clone failed. 
	exit
fi

cp -r debian ${workdir}/debian

# VOIP tweak  
cp ${workdir}/src/retroshare-gui/src/gui/chat/PopupChatDialog.ui ${workdir}/src/plugins/VOIP/gui/PopupChatDialog.ui

# Cloning sqlcipher
# git clone https://github.com/sqlcipher/sqlcipher.git

cd ${workdir}
echo Setting version numbers...

# setup version numbers
sed -e "s%RS_REVISION_NUMBER.*%RS_REVISION_NUMBER   0x${hhsh}%"  src/libretroshare/src/retroshare/rsversion.in > src/libretroshare/src/retroshare/rsversion.h

# Various cleaning
echo Cleaning...

\rm -rf src/.git

echo Calling debuild...
for i in ${dist}; do
    echo copying changelog for ${i}
    sed -e s/XXXXXX/"${rev}"/g -e s/YYYYYY/"${i}"/g ../changelog > debian/changelog

    if test -f ../control."${i}" ; then
		echo \/\!\\ Using specific control file for distribution "${i}"
        cp ../control."${i}" debian/control
    else
		echo Using standard control file control."${i}" for distribution "${i}"
        cp ../debian/control debian/control
    fi

    debuild -S -k${gpgkey}
done
cd -

exit 0
