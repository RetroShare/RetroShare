#!/bin/sh

###################### PARAMETERS ####################
gitpath="https://github.com/csoler/RetroShare.git"
#branch="master"
branch="v0.6.5-DebianPackaging"
#bubba3="Y"      # comment out to compile for bubba3
######################################################

RS_MAJOR_VERSION=0
RS_MINOR_VERSION=6
RS_BUILD_NUMBER=5

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
        "-key") shift
            gpgkey=${1}
            shift
            ;;
		  "-nodl")
		  		nodl=yes
				shift
				;;
		  "-makeorig") 
		  		makeorig=yes
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

removeIrrelevantFiles() {
	echo Removing irrelevant files in directory ${workdir}...
	# remove unised qml code, only needed on Android
	rm -rf ${workdir}/src/retroshare-qml-app/
	rm -rf ${workdir}/src/librssimulator/
	rm -rf ${workdir}/src/libretroshare/src/tests/
	rm -rf ${workdir}/src/libretroshare/src/unfinished/
	rm -rf ${workdir}/src/libretroshare/unused/
	rm -rf ${workdir}/src/retroshare-android-notify-service/
	rm -rf ${workdir}/src/retroshare-android-service/
	rm -rf ${workdir}/src/libretroshare/src/unused/
	rm -rf ${workdir}/src/supportlibs/
	rm -rf ${workdir}/src/retroshare-service/
	rm -rf ${workdir}/src/plugins/
	rm -rf ${workdir}/src/unittests/
	rm -rf ${workdir}/src/tests/
	rm -rf ${workdir}/src/build_scripts/
	rm -rf ${workdir}/src/libbitdht/src/tests/
	rm -rf ${workdir}/src/libbitdht/src/example/
	rm -rf ${workdir}/src/retroshare-gui/src/gui/WikiPoos/
	rm -rf ${workdir}/src/retroshare-gui/src/Unused/
	rm -f ${workdir}/debian/*~
	rm -f ${workdir}/debian/.*.sw?
	rm -f ${workdir}/src/retroshare-gui/src/gui/qss/chat/Bubble_Compact/private/images.sh
	rm -f ${workdir}/src/retroshare-gui/src/gui/qss/chat/Bubble/src/images.sh
	rm -f ${workdir}/src/retroshare-gui/src/gui/qss/chat/Bubble/public/images.sh
	rm -f ${workdir}/src/retroshare-gui/src/gui/qss/chat/Bubble/"history"/images.sh
	rm -f ${workdir}/src/retroshare-gui/src/gui/qss/chat/Bubble/private/images.sh
}

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

if ! test "${makeorig}" = "yes" ; then
	if ! test -f retroshare_${version_number}.orig.tar.gz; then
		echo Error: no orig file found. Please call with -makeorig option first
		exit
	fi
fi

if ! test "${nodl}" = "yes"; then
	mkdir -p ${workdir}/src
	echo Checking out latest snapshot...
	cd ${workdir}/src
	git clone --depth 1 ${gitpath} --single-branch --branch $branch .
	
	cd -

	if ! test -d ${workdir}/src/libretroshare/; then
	   echo Git clone failed. 
	   exit
	fi

	cp -r debian ${workdir}/debian

	# VOIP tweak  
	cp ${workdir}/src/retroshare-gui/src/gui/chat/PopupChatDialog.ui ${workdir}/src/plugins/VOIP/gui/PopupChatDialog.ui

	removeIrrelevantFiles

	cd ${workdir}
	echo Setting version numbers...

	# setup version numbers
	# sed -e "s%RS_REVISION_NUMBER.*%RS_REVISION_NUMBER   0x${hhsh}%"  src/libretroshare/src/retroshare/rsversion.in > src/libretroshare/src/retroshare/rsversion.h

	# Various cleaning
	echo Cleaning...

	\rm -rf src/.git

	if test "${makeorig}" = "yes" ; then
		echo making orig archive
		cd -
		tar zcvf retroshare_${version_number}.orig.tar.gz ${workdir}
		exit
	fi

	cd -
else
	tar zxvf retroshare_${version_number}.orig.tar.gz

	cp -r debian/* ${workdir}/debian/
	removeIrrelevantFiles
fi

# Cloning sqlcipher
# git clone https://github.com/sqlcipher/sqlcipher.git

echo Calling debuild...
cd ${workdir}
debuild -S -k${gpgkey} --lintian-opts +pedantic -EviIL
cd -

exit 0
