#!/bin/sh

###################### PARAMETERS ####################
gitpath="https://github.com/RetroShare/RetroShare.git"
branch="master"
#branch="v0.6.4-official_release"
#bubba3="Y"      # comment out to compile for bubba3
######################################################

RS_MAJOR_VERSION=0
RS_MINOR_VERSION=6
RS_BUILD_NUMBER=4

#  echo "RS_MAJOR_VERSION="${RS_MAJOR_VERSION}
#  echo "RS_MINOR_VERSION="${RS_MINOR_VERSION}
#  echo "RS_BUILD_NUMBER="${RS_BUILD_NUMBER}

version_number="${RS_MAJOR_VERSION}"'.'"${RS_MINOR_VERSION}"'.'"${RS_BUILD_NUMBER}"
workdir=retroshare-${version_number}

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
hhsh=`git log --pretty=format:"%H"  | head -1 | cut -c1-8`

rev=${date}.${hhsh}
useretrotor="false"

while [ ${#} -gt 0 ]; do
    case ${1} in
        "-rev") shift
            rev=${1}
            shift
            ;;
        "-retrotor") shift
           useretrotor="true"
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

if test "${useretrotor}" = "true"; then
   if ! test "${dist}" = "trusty"; then
      echo ERROR: retro-tor can only be packaged for trusty for now.
      exit 1;
   fi
   #gitpath="https://github.com/csoler/RetroShare.git"
   #branch="v0.6-TorOnly"
fi

if test "${dist}" = "" ; then
   dist="trusty xenial artful bionic"
fi

echo Attempting to get revision number...
ccount=`git rev-list --count --all`
ccount=`expr $ccount + 8613 - 8267`

gitrev=`git describe | cut -d- -f2-3`

echo "  Workdir            :"${workdir}
echo "  Version            :"${version_number}
echo "  Using revision     :"${rev}
echo "  Git Revision       :"${gitrev}
echo "  Commit count       :"${ccount}
echo "  Hash               :"${hhsh}
echo "  Date               :"${date}
echo "  Time               :"${time}
echo "  Using branch       :"${branch}
echo "  Using distributions:"${dist}
echo "  Using PGP key id   :"${gpgkey}

if test ${useretrotor} = "true"; then
   echo "  "Specific flags     : retrotor
fi

echo Done.
version="${version_number}"."${rev}"
echo Got version number ${version} 
echo
echo Please check that the changelog is up to date. 
echo Hit ENTER if this is correct. Otherwise hit Ctrl+C 
read tmp

echo Extracting base archive...

mkdir -p ${workdir}/src
echo Checking out latest snapshot...
cd ${workdir}/src
git clone --depth 1 ${gitpath} --single-branch --branch $branch .

#  if ! test "$hhsh" = "" ; then
#     echo Checking out revision $hhsh
#     git checkout $hhsh
#  fi

cd -

if ! test -d ${workdir}/src/libretroshare/; then
   echo Git clone failed. 
   exit
fi

cp -r debian ${workdir}/debian

# VOIP tweak  
cp ${workdir}/src/retroshare-gui/src/gui/chat/PopupChatDialog.ui ${workdir}/src/plugins/VOIP/gui/PopupChatDialog.ui

# remove unised qml code, only needed on Android

rm -rf ${workdir}/src/retroshare-qml-app/
rm -rf ${workdir}/src/build_scripts/
rm     ${workdir}/debian/*~

# Cloning sqlcipher
# git clone https://github.com/sqlcipher/sqlcipher.git

cd ${workdir}
echo Setting version numbers...

# setup version numbers
#sed -e "s%RS_REVISION_NUMBER.*%RS_REVISION_NUMBER   0x${hhsh}%"  src/libretroshare/src/retroshare/rsversion.in > src/libretroshare/src/retroshare/rsversion.h

# Various cleaning
echo Cleaning...

\rm -rf src/.git

echo Calling debuild...
for i in ${dist}; do

    if ! test "${i}" = "debian"; then
      echo copying changelog for ${i}
      sed -e s/XXXXXX/"${rev}"/g -e s/YYYYYY/"${i}"/g -e s/ZZZZZZ/"${version_number}"/g ../changelog > debian/changelog
      sed -e s/XXXXXX/"-${gitrev}"/g debian/rules > debian_rules_tmp
		cp debian_rules_tmp debian/rules

      if test ${useretrotor} = "true"; then
         cp ../rules.retrotor debian/rules
         cp ../control.trusty_retrotor debian/control
      elif test -f ../control."${i}" ; then
        echo \/\!\\ Using specific control file for distribution "${i}"
        cp ../control."${i}" debian/control
      else
        echo Using standard control file control."${i}" for distribution "${i}"
        cp ../debian/control debian/control
      fi
    else
      echo creating official debian release. Using built-in changelog and control files
    fi

    debuild -S -k${gpgkey} --lintian-opts +pedantic -EviIL
done
cd -

exit 0
