#!/bin/bash

# Script to pull debugging sysroot from Android devices
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

Pull files from Android device to prepare a debugging sysroot
Inspired by:
https://fw4spl-org.github.io/fw4spl-blog/2015/07/27/Native-debugging-on-Android-with-QtCreator.html

The goal is to have a local copy of the sysroot of the connected device.
For that we use the command `adb pull <remote_dir_or_file> <local_dir_or_file>`

We will get a copy of:

- `/system/lib/`
- `/vendor/lib/`
- `libc.so`
- `libcutils.so`

As well as the binaries `linker` and `app_process`

IMPORTANT:
from one device to another, the remote file `app_process` can be a binary file
or a symlink to a binary file - which is NOT ok for us.
That's so we will try to pull every known possible variants of `app_process`,
e.g. `app_process32` or `app_process_init`

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

define_default_value ANDROID_SERIAL "$(adb devices | head -n 2 | tail -n 1 | awk '{print $1}')"
define_default_value DEBUG_SYSROOT "${HOME}/Builds/debug_sysroot/${ANDROID_SERIAL}/"

rm -rf "${DEBUG_SYSROOT}"

for mDir in  "/system/lib/" "/vendor/lib/"; do
	mkdir -p "${DEBUG_SYSROOT}/${mDir}"
	# adb pull doesn't behave like rsync dirA/ dirB/ so avoid nesting the
	# directory by deleting last one before copying
	rmdir "${DEBUG_SYSROOT}/${mDir}"
	adb pull "${mDir}" "${DEBUG_SYSROOT}/${mDir}"
done

# Retrieve the specific binaries - some of these adb commands will fail, since
# some files may not exist, but that's ok.

mkdir -p "${DEBUG_SYSROOT}/system/bin/"
for mBin in "/system/bin/linker" "/system/bin/app_process" \
	"/system/bin/app_process_init" "/system/bin/app_process32" \
	"/system/bin/app_process64" ; do
	adb pull "${mBin}" "${DEBUG_SYSROOT}/${mBin}"
done

# Verify which variants of the specific binaries could be pulled 
echo
echo "Found the following specific binaries:"
echo
ls -1 "${DEBUG_SYSROOT}/system/bin/"*
ls -1 "${DEBUG_SYSROOT}/system/lib/libc.so"*
ls -1 "${DEBUG_SYSROOT}/system/lib/libcutils.so"*
echo

echo DEBUG_SYSROOT="${DEBUG_SYSROOT}"
