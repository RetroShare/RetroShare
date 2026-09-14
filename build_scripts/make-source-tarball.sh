#!/bin/bash

# make-source-tarball.sh — build a self-contained RetroShare source tarball.
#
# SPDX-License-Identifier: AGPL-3.0-or-later
#
# Produces a tarball that builds OFFLINE with CMake: it embeds every
# initialized git submodule at its checked-out commit and stamps the version
# in Source_Version files so the CMake configure works without any git
# metadata (see the Source_Version fallbacks in libretroshare/CMakeLists.txt
# and retroshare-gui/CMakeLists.txt). Intended for distribution source
# packages (e.g. Debian), where network access at build time is forbidden;
# such builds should configure with -DRS_FETCH_MISSING_DEPS=OFF so any
# missing dependency aborts the configure instead of being downloaded.
#
# Usage: run from anywhere inside the super-project checkout:
#
#   ./build_scripts/make-source-tarball.sh [OUTPUT_DIR]
#
# Environment:
#   RS_TARBALL_ALLOW_DIRTY=1   allow tracked modifications / out-of-sync
#                              submodules (the tarball then reflects the
#                              working tree commits, not a tagged release)
#   RS_TARBALL_COMPRESS=xz|gz  compression (default xz)
#
# The tarball is named retroshare_<version>.tar.<ext> where <version> is the
# super-project `git describe` with the leading 'v' dropped and the dashes
# after the tag turned into dots (same convention as the historical OBS
# packaging scripts).

set -e

TOP_DIR="$(git rev-parse --show-toplevel)"
OUT_DIR="${1:-$(pwd)}"
OUT_DIR="$(cd "${OUT_DIR}" && pwd)"
COMPRESS="${RS_TARBALL_COMPRESS:-xz}"

cd "${TOP_DIR}"

## Sanity: refuse silently wrong content unless explicitly allowed.

DIRTY="$(git status --porcelain --untracked-files=no)"
if [ -n "${DIRTY}" ] && [ "${RS_TARBALL_ALLOW_DIRTY}" != "1" ]; then
	echo "ERROR: the working tree has tracked modifications:" >&2
	echo "${DIRTY}" >&2
	echo "A release tarball must be built from a clean checkout of the release tag." >&2
	echo "Set RS_TARBALL_ALLOW_DIRTY=1 to override (testing only)." >&2
	exit 1
fi

MISSING="$(git submodule status --recursive | grep '^-' || true)"
if [ -n "${MISSING}" ]; then
	echo "ERROR: uninitialized submodules, the tarball would not be self-contained:" >&2
	echo "${MISSING}" >&2
	echo "Initialize them first (see AGENTS.md / BUILD-cmake.md: init WITHOUT --remote)." >&2
	exit 1
fi

OUTOFSYNC="$(git submodule status --recursive | grep '^+' || true)"
if [ -n "${OUTOFSYNC}" ] && [ "${RS_TARBALL_ALLOW_DIRTY}" != "1" ]; then
	echo "ERROR: submodules checked out on a different commit than the recorded gitlink:" >&2
	echo "${OUTOFSYNC}" >&2
	echo "Run 'git submodule update' first, or set RS_TARBALL_ALLOW_DIRTY=1 (testing only)." >&2
	exit 1
fi

## Version stamps, computed while git metadata is still around.

ROOT_DESCRIBE="$(git describe --tags --always)"
LIB_DESCRIBE="$(git -C libretroshare describe --tags --long --match 'v*.*.*')"
VERSION="$(echo "${ROOT_DESCRIBE}" | sed -e 's/-/./2g' | sed -e 's/^v//')"

echo "Super-project version: ${ROOT_DESCRIBE}"
echo "libretroshare version: ${LIB_DESCRIBE}"
echo "Tarball version:       ${VERSION}"

## Stage the super-project then overlay every submodule at its own commit.
## git archive honours export-ignore attributes and never includes .git.

STAGE_ROOT="$(mktemp --directory)"
trap 'rm -rf "${STAGE_ROOT}"' EXIT
STAGE="${STAGE_ROOT}/retroshare-${VERSION}"
mkdir "${STAGE}"

echo "Staging super-project ..."
git archive HEAD | tar -x -C "${STAGE}"

git submodule status --recursive | sed -e 's/^[ +]//' | while read -r SM_COMMIT SM_PATH SM_REST; do
	echo "Staging submodule ${SM_PATH} @ ${SM_COMMIT} ..."
	rm -rf "${STAGE:?}/${SM_PATH}"
	mkdir -p "${STAGE}/${SM_PATH}"
	git -C "${SM_PATH}" archive "${SM_COMMIT}" | tar -x -C "${STAGE}/${SM_PATH}"
done

echo "${ROOT_DESCRIBE}" > "${STAGE}/Source_Version"
echo "${LIB_DESCRIBE}"  > "${STAGE}/libretroshare/Source_Version"

## Pack.

case "${COMPRESS}" in
	xz) TAR_FLAGS="-cJf"; EXT="tar.xz" ;;
	gz) TAR_FLAGS="-czf"; EXT="tar.gz" ;;
	*) echo "ERROR: unsupported RS_TARBALL_COMPRESS=${COMPRESS} (xz|gz)" >&2; exit 1 ;;
esac

TARBALL="${OUT_DIR}/retroshare_${VERSION}.${EXT}"
echo "Packing ${TARBALL} ..."
tar -C "${STAGE_ROOT}" ${TAR_FLAGS} "${TARBALL}" "retroshare-${VERSION}"

echo "Done: ${TARBALL}"
echo "Offline configure check: extract it, then"
echo "  cmake -S retroshare-${VERSION} -B build -DRS_FETCH_MISSING_DEPS=OFF"
