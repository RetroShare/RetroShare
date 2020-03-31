#!/bin/bash

# Script to start gdbserver on Android device and attach to retroshare-service
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

<<"ASCIIDOC"

Start gdbserver on Android device and attach it to the retroshare service
process

Inspired by:
https://fw4spl-org.github.io/fw4spl-blog/2015/07/27/Native-debugging-on-Android-with-QtCreator.html

ASCIIDOC


## Define default value for variable, take two arguments, $1 variable name,
## $2 default variable value, if the variable is not already define define it
## with default value.
function define_default_value()
{
	VAR_NAME="${1}"
	DEFAULT_VALUE="${2}"

	[ -z "${!VAR_NAME}" ] && export ${VAR_NAME}="${DEFAULT_VALUE}"
}

define_default_value ANDROID_APK_PACKAGE "org.retroshare.service"
define_default_value ANDROID_PROCESS_NAME "org.retroshare.service:rs"
define_default_value ANDROID_SERIAL "$(adb devices | head -n 2 | tail -n 1 | awk '{print $1}')"
define_default_value GDB_SERVER_PORT 5039

adb_shell()
{
	adb shell run-as ${ANDROID_APK_PACKAGE} $@
}

## If not passed as environement variable try to determine gdbserver path
## shipped withing Android package
[ -z "${LIB_GDB_SERVER_PATH}" ] &&
{
	for mUsualPath in \
		"/data/data/${ANDROID_APK_PACKAGE}/lib/libgdbserver.so" \
		"/data/app/${ANDROID_APK_PACKAGE}"*"/lib/arm64/libgdbserver.so"
	do
		adb_shell ls "${mUsualPath}" &&
			export LIB_GDB_SERVER_PATH="${mUsualPath}" && break
	done
}


[ -z "${LIB_GDB_SERVER_PATH}" ] &&
{
	cat <<EOF
libgdbserver.so not found in any of the usual path attempting to look for it
with find, it will take a bunch of time. Take note of the discovered path and
define LIB_GDB_SERVER_PATH on your commandline at next run to avoid waiting
again.


EOF
	tFile="$(mktemp)"
	adb_shell find -type f -name 'libgdbserver.so' / | \
		grep ${ANDROID_APK_PACKAGE} | tee "${tFile}"

	LIB_GDB_SERVER_PATH="$(head -n 1 "${tFile}")"
	rm "${tFile}"
}

[ -z "${LIB_GDB_SERVER_PATH}" ] &&
{
	echo "Cannot find libgdbserver.so, are you sure your package ships it?"
	exit -1
}

mPid="$(adb_shell ps | grep ${ANDROID_PROCESS_NAME} | awk '{print $2}')"
[ -z "${mPid}" ] &&
{
	echo "Failed ${ANDROID_PROCESS_NAME} PID retrival are you sure it is running?"
	exit -2
}


## Establish port forwarding so we can connect to gdbserver with gdb
adb forward tcp:${GDB_SERVER_PORT} tcp:${GDB_SERVER_PORT}

((adb_shell ${LIB_GDB_SERVER_PATH} 127.0.0.1:${GDB_SERVER_PORT} --attach ${mPid})&)
