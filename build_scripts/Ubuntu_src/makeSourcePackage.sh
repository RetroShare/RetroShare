#!/bin/sh

###################### PARAMETERS ####################
version="0.6.0"
svnpath="svn://csoler@svn.code.sf.net/p/retroshare/code/"
workdir=retroshare06-$version
#bubba3="Y"		# comment out to compile for bubba3
######################################################

echo This script is going to build the debian source package for RetroShare, from the svn.

if test -d "$workdir" ;  then
	echo Removing the $workdir directory...
	rm -rf $workdir
fi

svn update

# Parse options
svnrev=""
dist=""
# This is the key for "Cyril Soler <csoler@sourceforge.net>"
gpgkey="C737CA98"
while [ $# -gt 0 ]; do
    case $1 in
	"-rev") shift
            svnrev=$1
	    shift
	    ;;
	"-distribution") shift
	    dist=$1
	    shift
	    ;;
	"-key") shift
	    gpgkey=$1
	    shift
	    ;;
	"*") echo "Unknown option"
	    exit 1
	    ;;
    esac
done

if test "$svnrev" = "" ; then
	echo attempting to get svn revision number...
	svn=`svn info | grep 'Revision:' | cut -d\  -f2`
else
	echo svn number has been provided. Forcing update.
fi

echo done.
version="$version"."$svnrev"
echo got version number $version. 
echo Please check that the changelog is up to date. 
echo Hit ENTER if this is correct. Otherwise hit Ctrl+C 
read tmp

packages="."

echo SVN number is $svnrev
echo version is $version

echo Extracting base archive...

mkdir -p $workdir/src
cp -r data   $workdir/src/
cp -r debian $workdir/debian

echo Checking out latest snapshot in libbitdht...
cd $workdir/src/
svn co -r$svnrev $svnpath/trunk/ . 
cd ../..

# VOIP tweak  
cp $workdir/src/retroshare-gui/src/gui/chat/PopupChatDialog.ui $workdir/src/plugins/VOIP/gui/PopupChatDialog.ui

# handling of libssh
#LIBSSH_VERSION=0.5.4
#LIBSSH_DIR=41
LIBSSH_VERSION=0.6.4
LIBSSH_DIR=107
[ -f libssh-${LIBSSH_VERSION}.tar.gz ] || wget --no-check-certificate -O libssh-${LIBSSH_VERSION}.tar.gz https://red.libssh.org/attachments/download/${LIBSSH_DIR}/libssh-${LIBSSH_VERSION}.tar.gz
cd $workdir
tar zxvf ../libssh-${LIBSSH_VERSION}.tar.gz
cd ..

# cd $workdir
# git clone https://github.com/sqlcipher/sqlcipher.git
# cd ..

# cleaning up protobof generated files
\rm -f $workdir/src/retroshare-nogui/src/rpc/proto/gencc/*.pb.h
\rm -f $workdir/src/retroshare-nogui/src/rpc/proto/gencc/*.pb.cc

echo Setting version numbers...

# setup version numbers
cat $workdir/src/libretroshare/src/util/rsversion.h | grep -v SVN_REVISION | grep -v SVN_REVISION_NUMBER > /tmp/toto2342
echo \#define SVN_REVISION \"Revision: "$version"  date : `date`\" >> /tmp/toto2342
echo \#define SVN_REVISION_NUMBER $svnrev >> /tmp/toto2342
cp /tmp/toto2342 $workdir/src/libretroshare/src/util/rsversion.h

cat $workdir/src/retroshare-gui/src/util/rsguiversion.h | grep -v GUI_REVISION | grep -v GUI_VERSION > /tmp/toto4463
echo \#define GUI_REVISION \"Revision: "$version"  date : `date`\" >> /tmp/toto4463
echo \#define GUI_VERSION \"Revision: "$svnrev"\" >> /tmp/toto4463
cp /tmp/toto4463 $workdir/src/retroshare-gui/src/util/rsguiversion.h

# Various cleaning
echo Cleaning...
find $workdir -name ".svn" -exec rm -rf {} \;		# remove all svn repositories

#echo Calling debuild...
#cat $workdir/debian/control | sed -e s/XXXXXX/"$version"/g > $workdir/debian/control.tmp
#mv -f $workdir/debian/control.tmp $workdir/debian/control

cd $workdir

for i in $dist; do
	echo copying changelog for $i
	sed -e s/XXXXXX/"$svnrev"/g -e s/YYYYYY/"$i"/g ../changelog > debian/changelog

	if test "$i" = "lucid" ; then
		cp ../control.ubuntu_lucid debian/control
	elif test "$i" = "squeeze" ; then
		cp ../control.squeeze_bubba3 debian/control
	else
		cp ../debian/control debian/control
	fi

	debuild -S -k$gpgkey
done
