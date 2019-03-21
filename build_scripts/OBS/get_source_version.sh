#!/bin/bash

SOURCE_VERSION_FILE="$( dirname "${BASH_SOURCE[0]}" )/../../Source_Version"

VERSION_MAJOR="$(cat "${SOURCE_VERSION_FILE}" | awk -F. '{print $1}')"
VERSION_MAJOR="${VERSION_MAJOR:1}"

VERSION_MINOR="$(cat "${SOURCE_VERSION_FILE}" | awk -F. '{print $2}')"

VERSION_MINI="$(cat "${SOURCE_VERSION_FILE}" | awk -F. '{print $3}' | awk -F- '{print $1}')"

VERSION_EXTRA="$(cat "${SOURCE_VERSION_FILE}" | awk -F- '{print "-"$2"-"$3"-OBS"}')"

echo RS_MAJOR_VERSION=${VERSION_MAJOR} RS_MINOR_VERSION=${VERSION_MINOR} RS_MINI_VERSION=${VERSION_MINI} RS_EXTRA_VERSION=\"${VERSION_EXTRA}\"
