#!/bin/bash

# Copyright (C) 2021  Gioacchino Mazzurco <gio@eigenlab.org>
# Copyright (C) 2021  Asociación Civil Altermundi <info@altermundi.net>
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
#
# SPDX-FileCopyrightText: Retroshare Team <contact@retroshare.cc>
# SPDX-License-Identifier: AGPL-3.0-only


## Define default value for variable, take two arguments, $1 variable name,
## $2 default variable value, if the variable is not already define define it
## with default value.
function define_default_value()
{
	VAR_NAME="${1}"
	DEFAULT_VALUE="${2}"

	[ -z "${!VAR_NAME}" ] && export ${VAR_NAME}="${DEFAULT_VALUE}" || true
}

define_default_value API_BASE_URL "http://127.0.0.1:9092"
define_default_value API_TOKEN "0000:0000"

function tLog()
{
	local mCategory="$1" ; shift
	echo "$mCategory $(date) $@" >&2
}

mCmd="curl -u $API_TOKEN $API_BASE_URL/rsFiles/requestDirDetails"

mReply="$($mCmd)"
mCurlRet="$?"

if [ "$mCurlRet" != 0 ]; then
	tLog E "$mCmd failed: $mCurlRet '$mReply'"
	exit -3
fi

[ "$(echo "$mReply" | jq '.retval')" == "true" ] ||
{
	tLog E "/rsFiles/requestDirDetails failed: '$mReply'"
	exit -1
}

[ "$(echo "$mReply" | jq '.details.handle')" != "0" ] ||
{
	tLog E ".details.handle has wrong type int: '$mReply'"
	exit -1
}

[ "$(echo "$mReply" | jq '.details.handle.xstr64')" == "\"0\"" ] &&
	[ "$(echo "$mReply" | jq '.details.handle.xint64')" == "0" ] &&
{
	tLog I '.details.handle has correct type'
	exit 0
}

tLog E "Unkown error in test $0: '$mReply'"
exit -2
