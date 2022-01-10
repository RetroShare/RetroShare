#!/bin/bash

# Script to prepare Android NDK GDB configurations to debug retroshare-service
#
# Copyright (C) 2020  Gioacchino Mazzurco <gio@eigenlab.org>
# Copyright (C) 2020  Asociación Civil Altermundi <info@altermundi.net>
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU Affero General Public License as published by the
# Free Software Foundation, version 3.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License along
# with this program. If not, see <https://www.gnu.org/licenses/>


## Define default value for variable, take two arguments, $1 variable name,
## $2 default variable value, if the variable is not already define define it
## with default value.
function define_default_value()
{
	VAR_NAME="${1}"
	DEFAULT_VALUE="${2}"

	[ -z "${!VAR_NAME}" ] && export ${VAR_NAME}="${DEFAULT_VALUE}"
}

define_default_value QT_VERSION "5.12.4"
define_default_value QT_ARCH "arm64_v8a"
define_default_value QT_DIR "/opt/Qt-${QT_VERSION}/${QT_VERSION}/"
define_default_value ANDROID_SERIAL "$(adb devices | head -n 2 | tail -n 1 | awk '{print $1}')"
define_default_value RS_BUILD_DIR "${HOME}/Builds/RetroShare-Android_for_${QT_ARCH}_Clang_Qt_${QT_VERSION//./_}_android_${QT_ARCH}-Debug/"
define_default_value RS_SOURCE_DIR "${HOME}/Development/rs-develop/"
define_default_value DEBUG_SYSROOT "${HOME}/Builds/debug_sysroot/${ANDROID_SERIAL}/"
define_default_value GDB_CONFIGS_FILE "${HOME}/Builds/gdb_configs_${QT_ARCH}"

scanDir()
{
	find "$1" -type d -not -path '*/\.git/*' | tr '\n' ':' >> $GDB_CONFIGS_FILE
}

putSeparator()
{
	echo >> $GDB_CONFIGS_FILE
	echo >> $GDB_CONFIGS_FILE
}

echo "set sysroot ${DEBUG_SYSROOT}" > $GDB_CONFIGS_FILE
putSeparator

echo "set auto-solib-add on" >> $GDB_CONFIGS_FILE
echo -n "set solib-search-path " >> $GDB_CONFIGS_FILE
scanDir "${RS_BUILD_DIR}"
scanDir "${DEBUG_SYSROOT}"
scanDir "${QT_DIR}/android_${QT_ARCH}/lib/"
scanDir "${QT_DIR}/android_${QT_ARCH}/plugins/"
scanDir "${QT_DIR}/android_${QT_ARCH}/qml/"
putSeparator

echo -n "directory " >> $GDB_CONFIGS_FILE
scanDir ${RS_SOURCE_DIR}/jsonapi-generator/src
scanDir ${RS_SOURCE_DIR}/libbitdht/src
scanDir ${RS_SOURCE_DIR}/openpgpsdk/src
scanDir ${RS_SOURCE_DIR}/libretroshare/src
scanDir ${RS_SOURCE_DIR}/retroshare-service/src
scanDir ${RS_SOURCE_DIR}/supportlibs/rapidjson/include/
scanDir ${RS_SOURCE_DIR}/supportlibs/restbed/source/
scanDir ${RS_SOURCE_DIR}/supportlibs/udp-discovery-cpp/
scanDir ${RS_SOURCE_DIR}/supportlibs/restbed/dependency/asio/asio/include/
scanDir ${RS_SOURCE_DIR}/supportlibs/restbed/dependency/catch/include/
putSeparator

## see https://stackoverflow.com/questions/28972367/gdb-backtrace-without-stopping
echo "catch signal SIGSEGV" >> $GDB_CONFIGS_FILE
echo "commands
   bt
   continue
   end" >> $GDB_CONFIGS_FILE
putSeparator

echo GDB_CONFIGS_FILE=$GDB_CONFIGS_FILE
