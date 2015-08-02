#!/bin/bash

#don't exit even if a command fails
set +e

source ../../get_version.sh

echo "Writing version to retroshare/rsautoversion.h : ${version}"
echo "#define RS_REVISION_NUMBER $version" > retroshare/rsautoversion.h
echo "#define RS_GIT_BRANCH \"$gitbranch\"" >> retroshare/rsautoversion.h
echo "#define RS_GIT_INFO \"$gitinfo\"" >> retroshare/rsautoversion.h
echo "#define RS_GIT_HASH \"$githash\"" >> retroshare/rsautoversion.h

echo "version_detail_lib.sh script finished"
exit 0
