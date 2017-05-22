#!/bin/sh

########################################################
# This script make the debian packages for all Debian
# and all architectures specified in the variables below:

distribs="stretch jessie"
archis="armhf amd64 i386"

########################################################

./clean.sh
rm -rf retroshare06-0.6.2
./makeSourcePackage.sh -distribution "$distribs"

for dist in $distribs; do
	for arch in $archis; do
		pbuilder-dist $dist $arch build retroshare06_0.6.2-1.???????.????????~"$dist".dsc
	done
done


