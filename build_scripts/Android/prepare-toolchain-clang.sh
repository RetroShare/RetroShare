#!/bin/bash

# Script to prepare RetroShare Android package building toolchain
#
# Copyright (C) 2016-2021  Gioacchino Mazzurco <gio@eigenlab.org>
# Copyright (C) 2020-2021  Asociación Civil Altermundi <info@altermundi.net>
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

## You are supposed to provide the following variables according to your system setup
define_default_value ANDROID_NDK_PATH "/opt/android-ndk/"
define_default_value ANDROID_NDK_ARCH "arm"
define_default_value ANDROID_PLATFORM_VER "16"
define_default_value NATIVE_LIBS_TOOLCHAIN_PATH "${HOME}/Builds/android-toolchains/retroshare-android-${ANDROID_PLATFORM_VER}-${ANDROID_NDK_ARCH}/"
define_default_value HOST_NUM_CPU $(nproc)

define_default_value ANDROID_SDK_INSTALL "false"
define_default_value ANDROID_SDK_TOOLS_VERSION "3859397"
define_default_value ANDROID_SDK_TOOLS_SHA256 444e22ce8ca0f67353bda4b85175ed3731cae3ffa695ca18119cbacef1c1bea0
define_default_value ANDROID_SDK_VERSION "29.0.3"

define_default_value ANDROID_NDK_INSTALL "false"
define_default_value ANDROID_NDK_VERSION "r21"
define_default_value ANDROID_NDK_SHA256 b65ea2d5c5b68fb603626adcbcea6e4d12c68eb8a73e373bbb9d23c252fc647b

define_default_value BZIP2_SOURCE_VERSION "1.0.6"
define_default_value BZIP2_SOURCE_SHA256 a2848f34fcd5d6cf47def00461fcb528a0484d8edef8208d6d2e2909dc61d9cd

define_default_value OPENSSL_SOURCE_VERSION "1.1.1c"
define_default_value OPENSSL_SOURCE_SHA256 f6fb3079ad15076154eda9413fed42877d668e7069d9b87396d0804fdb3f4c90

define_default_value SQLITE_SOURCE_YEAR "2018"
define_default_value SQLITE_SOURCE_VERSION "3250200"
define_default_value SQLITE_SOURCE_SHA256 da9a1484423d524d3ac793af518cdf870c8255d209e369bd6a193e9f9d0e3181

define_default_value SQLCIPHER_SOURCE_VERSION "4.4.3"
define_default_value SQLCIPHER_SOURCE_SHA256 b8df69b998c042ce7f8a99f07cf11f45dfebe51110ef92de95f1728358853133

define_default_value LIBUPNP_SOURCE_VERSION "1.8.4"
define_default_value LIBUPNP_SOURCE_SHA256 976c3e4555604cdd8391ed2f359c08c9dead3b6bf131c24ce78e64d6669af2ed

define_default_value QT_ANDROID_VIA_INSTALLER "false"
define_default_value QT_VERSION "5.12.11"
define_default_value QT_INSTALLER_VERSION "4.1.1"
define_default_value QT_INSTALLER_SHA256 1266ffd0d1b0e466244e3bc8422975c1aa9d96745b6bb28d422f7f92df11f34c
define_default_value QT_INSTALLER_JWT_TOKEN "Need a QT account JWT token to use the insaller see https://wiki.qt.io/Online_Installer_4.x"
define_default_value QT_INSTALL_PATH "${NATIVE_LIBS_TOOLCHAIN_PATH}/Qt/"

define_default_value QT_ANDROID_INSTALLER_SHA256 a214084e2295c9a9f8727e8a0131c37255bf724bfc69e80f7012ba3abeb1f763

define_default_value RESTBED_SOURCE_REPO "https://github.com/Corvusoft/restbed.git"
define_default_value RESTBED_SOURCE_VERSION f74f9329dac82e662c1d570b7cd72c192b729eb4

define_default_value UDP_DISCOVERY_CPP_SOURCE "https://github.com/truvorskameikin/udp-discovery-cpp.git"
define_default_value UDP_DISCOVERY_CPP_VERSION "develop"

define_default_value XAPIAN_SOURCE_VERSION "1.4.7"
define_default_value XAPIAN_SOURCE_SHA256 13f08a0b649c7afa804fa0e85678d693fd6069dd394c9b9e7d41973d74a3b5d3

define_default_value RAPIDJSON_SOURCE_VERSION "1.1.0"
define_default_value RAPIDJSON_SOURCE_SHA256 bf7ced29704a1e696fbccf2a2b4ea068e7774fa37f6d7dd4039d0787f8bed98e

define_default_value MINIUPNPC_SOURCE_VERSION "2.1.20190625"
define_default_value MINIUPNPC_SOURCE_SHA256 8723f5d7fd7970de23635547700878cd29a5c2bb708b5e5475b2d1d2510317fb

# zlib and libpng versions walks toghether
define_default_value ZLIB_SOURCE_VERSION "1.2.11"
define_default_value ZLIB_SOURCE_SHA256 4ff941449631ace0d4d203e3483be9dbc9da454084111f97ea0a2114e19bf066

define_default_value LIBPNG_SOURCE_VERSION "1.6.37"
define_default_value LIBPNG_SOURCE_SHA256 505e70834d35383537b6491e7ae8641f1a4bed1876dbfe361201fc80868d88ca

define_default_value LIBJPEG_SOURCE_VERSION "9d"
define_default_value LIBJPEG_SOURCE_SHA256 6c434a3be59f8f62425b2e3c077e785c9ce30ee5874ea1c270e843f273ba71ee

define_default_value TIFF_SOURCE_VERSION "4.2.0"
define_default_value TIFF_SOURCE_SHA256 eb0484e568ead8fa23b513e9b0041df7e327f4ee2d22db5a533929dfc19633cb

define_default_value CIMG_SOURCE_VERSION "2.9.7"
define_default_value CIMG_SOURCE_SHA256 595dda9718431a123b418fa0db88e248c44590d47d9b1646970fa0503e27fa5c

define_default_value PHASH_SOURCE_REPO "https://gitlab.com/g10h4ck/pHash.git"
define_default_value PHASH_SOURCE_VERSION origin/android-ndk

define_default_value MVPTREE_SOURCE_REPO "https://github.com/starkdg/mvptree.git"
define_default_value MVPTREE_SOURCE_VERSION origin/master

define_default_value REPORT_DIR "$(pwd)/$(basename ${NATIVE_LIBS_TOOLCHAIN_PATH})_build_report/"

cArch=""
eABI=""
cmakeABI=""

case "${ANDROID_NDK_ARCH}" in
"arm")
	cArch="${ANDROID_NDK_ARCH}"
	eABI="eabi"
	;;
"arm64")
	cArch="aarch64"
	eABI=""
	;;
"x86")
	cArch="i686"
	eABI=""
	;;
"x86_64")
	echo "ANDROID_NDK_ARCH=${ANDROID_NDK_ARCH} not supported yet"
	exit -1
	cArch="??"
	eABI=""
esac

export SYSROOT="${NATIVE_LIBS_TOOLCHAIN_PATH}/sysroot/"
export PREFIX="${SYSROOT}/usr/"
export CC="${NATIVE_LIBS_TOOLCHAIN_PATH}/bin/${cArch}-linux-android${eABI}-clang"
export CXX="${NATIVE_LIBS_TOOLCHAIN_PATH}/bin/${cArch}-linux-android${eABI}-clang++"
export AR="${NATIVE_LIBS_TOOLCHAIN_PATH}/bin/${cArch}-linux-android${eABI}-ar"
export RANLIB="${NATIVE_LIBS_TOOLCHAIN_PATH}/bin/${cArch}-linux-android${eABI}-ranlib"

# Used to instruct cmake to explicitely ignore host libraries
export HOST_IGNORE_PREFIX="/usr/"


## $1 filename, $2 sha256 hash
function check_sha256()
{
	echo ${2} "${1}" | sha256sum -c &> /dev/null
}

## $1 filename, $2 sha256 hash, $3 url
function verified_download()
{
	FILENAME="$1"
	SHA256="$2"
	URL="$3"

	check_sha256 "${FILENAME}" "${SHA256}" ||
	{
		rm -rf "${FILENAME}"

		wget -O "${FILENAME}" "$URL" ||
		{
			echo "Failed downloading ${FILENAME} from $URL"
			exit 1
		}

		check_sha256 "${FILENAME}" "${SHA256}" ||
		{
			echo "SHA256 mismatch for ${FILENAME} from ${URL} expected sha256 ${SHA256} got $(sha256sum ${FILENAME} | awk '{print $1}')"
			exit 1
		}
	}
}

# This function is the result of reading and testing many many stuff be very
# careful editing it
function andro_cmake()
{
# Using android.toolchain.cmake as documented here
# https://developer.android.com/ndk/guides/cmake seens to break more things then
# it fixes :-\

	cmakeProc=""
	case "${ANDROID_NDK_ARCH}" in
	"arm")
		cmakeProc="armv7-a"
	;;
	"arm64")
		cmakeProc="aarch64"
	;;
	"x86")
		cmakeProc="i686"
	;;
	"x86_64")
		cmakeProc="x86_64"
	;;
	*)
		echo "Unhandled NDK architecture ${ANDROID_NDK_ARCH}"
		exit -1
	;;
	esac

	_hi="$HOST_IGNORE_PREFIX"

	cmake \
		-DCMAKE_SYSTEM_PROCESSOR=$cmakeProc \
		-DCMAKE_POSITION_INDEPENDENT_CODE=ON \
		-DCMAKE_PREFIX_PATH="${PREFIX}" \
		-DCMAKE_SYSTEM_PREFIX_PATH="${PREFIX}" \
		-DCMAKE_INCLUDE_PATH="${PREFIX}/include" \
		-DCMAKE_SYSTEM_INCLUDE_PATH="${PREFIX}/include" \
		-DCMAKE_LIBRARY_PATH="${PREFIX}/lib" \
		-DCMAKE_SYSTEM_LIBRARY_PATH="${PREFIX}/lib" \
		-DCMAKE_INSTALL_PREFIX="${PREFIX}" \
		-DCMAKE_IGNORE_PATH="$_hi/include;$_hi/lib;$_hi/lib64" \
		$@

	# It is probably ok to do not touch CMAKE_PROGRAM_PATH and
	# CMAKE_SYSTEM_PROGRAM_PATH
}

function git_source_get()
{
	sourceDir="$1" ; shift #$1
	sourceRepo="$1"  ; shift  #$2
	sourceVersion="$1"  ; shift  #$3
	# extra paramethers are treated as submodules

	[ -d "$sourceDir" ] &&
	{
		pushd "$sourceDir"
		actUrl="$(git remote get-url origin)"
		[ "$actUrl" != "$sourceRepo" ] && rm -rf "${sourceDir}"
		popd
	} || true

	[ -d $sourceDir ] || git clone "$sourceRepo" "$sourceDir"
	pushd $sourceDir
	
	git fetch --all
	git reset --hard ${sourceVersion}

	while [ "$1" != "" ] ; do
		git submodule update --init "$1"
		pushd "$1"
		git reset --hard
		shift
		popd
	done

	popd
}

declare -A TASK_REGISTER

function task_register()
{
	TASK_REGISTER[$1]=true
}

function task_unregister()
{
	# we may simply wipe them but we could benefit from keeping track of
	# unregistered tasks too
	TASK_REGISTER[$1]=false
}

function task_logfile()
{
	echo "$REPORT_DIR/$1.log"
}

function task_run()
{
	mTask="$1" ; shift

	[ "${TASK_REGISTER[$mTask]}" != "true" ] &&
	{
		echo "Attempt to run not registered task $mTask $@"
		return -1
	}

	logFile="$(task_logfile $mTask)"
	if [ -f "$logFile" ] ; then
		echo "Task $mTask already run more details at $logFile"
	else
		date | tee > "$logFile"
		$mTask $@ |& tee --append "$logFile"
		mRetval="${PIPESTATUS[0]}"
		echo "Task $mTask return ${mRetval} more details at $logFile"
		date | tee --append "$logFile"
		return ${mRetval}
	fi
}

function task_zap()
{
	rm -f "$(task_logfile $1)"
}

DUPLICATED_INCLUDES_LIST_FILE="${REPORT_DIR}/duplicated_includes_list"
DUPLICATED_INCLUDES_DIR="${REPORT_DIR}/duplicated_includes/"

task_register install_android_sdk
install_android_sdk()
{
	tFile="sdk-tools-linux-${ANDROID_SDK_TOOLS_VERSION}.zip"

	verified_download "${tFile}" "${ANDROID_SDK_TOOLS_SHA256}" \
		"https://dl.google.com/android/repository/${tFile}"

	unzip "${tFile}"
	mkdir -p "$ANDROID_SDK_PATH"
	rm -rf "$ANDROID_SDK_PATH/tools/"
	mv --verbose tools/ "$ANDROID_SDK_PATH/tools/"

	# Install Android SDK
	yes | $ANDROID_SDK_PATH/tools/bin/sdkmanager --licenses && \
		$ANDROID_SDK_PATH/tools/bin/sdkmanager --update
	$ANDROID_SDK_PATH/tools/bin/sdkmanager "platforms;android-$ANDROID_PLATFORM_VER"
	$ANDROID_SDK_PATH/tools/bin/sdkmanager "build-tools;$ANDROID_SDK_VERSION"
}

task_register install_android_ndk
install_android_ndk()
{
	tFile="android-ndk-${ANDROID_NDK_VERSION}-linux-x86_64.zip"

	verified_download "${tFile}" "${ANDROID_NDK_SHA256}" \
		"https://dl.google.com/android/repository/${tFile}"

	unzip "${tFile}"
	mkdir -p "$ANDROID_NDK_PATH"
	rm -rf "$ANDROID_NDK_PATH"
	mv --verbose  "android-ndk-${ANDROID_NDK_VERSION}/" "$ANDROID_NDK_PATH/"
}

## More information available at https://android.googlesource.com/platform/ndk/+/ics-mr0/docs/STANDALONE-TOOLCHAIN.html
task_register bootstrap_toolchain
bootstrap_toolchain()
{
	rm -rf "${NATIVE_LIBS_TOOLCHAIN_PATH}"
	${ANDROID_NDK_PATH}/build/tools/make_standalone_toolchain.py --verbose \
		--arch ${ANDROID_NDK_ARCH} --install-dir ${NATIVE_LIBS_TOOLCHAIN_PATH} \
		--api ${ANDROID_PLATFORM_VER}

	# Avoid problems with arm64 some libraries installing on lib64
	ln -s "${PREFIX}/lib/" "${PREFIX}/lib64"

	pushd "${PREFIX}/include/"
	find . -not -type d > "${DUPLICATED_INCLUDES_LIST_FILE}"
	popd
}

## This avoid <cmath> include errors due to -isystem and -I ordering issue
task_register deduplicate_includes
deduplicate_includes()
{
	while read delFile ; do
		mNewPath="${DUPLICATED_INCLUDES_DIR}/$delFile"
		mkdir --verbose --parents "$(dirname "$mNewPath")"
		mv --verbose "${PREFIX}/include/$delFile" "$mNewPath"
	done < "${DUPLICATED_INCLUDES_LIST_FILE}"
}

task_register reduplicate_includes
reduplicate_includes()
{
	pushd "${DUPLICATED_INCLUDES_DIR}"
	find . -not -type d | while read delFile ; do
		mv --verbose "${delFile}" "${PREFIX}/include/$delFile"
	done
	popd
}

# $1 optional prefix prepended only if return value is not empty
# $2 optional suffix appended only if return value is not empty
task_register get_qt_arch
get_qt_arch()
{
	local QT_VERSION_COMP="$(echo $QT_VERSION | awk -F. '{print $1*1000000+$2*1000+$3}')" 
	local QT_ARCH=""

	# Qt >= 5.15.0 ships all Android architectures toghether
	[ "$QT_VERSION_COMP" -lt "5015000" ] &&
	{
		case "${ANDROID_NDK_ARCH}" in
		"arm")
			QT_ARCH="armv7"
			;;
		"arm64")
			QT_ARCH="arm64_v8a"
			;;
		"x86")
			QT_ARCH="x86"
			;;
		esac

		echo "$1$QT_ARCH$2"
	}
}

task_register get_qt_dir
get_qt_dir()
{
	echo "${QT_INSTALL_PATH}/${QT_VERSION}/android$(get_qt_arch _)/"
}

## More information available at https://wiki.qt.io/Online_Installer_4.x
task_register install_qt_android
install_qt_android()
{
	QT_VERSION_CODE="$(echo $QT_VERSION | tr -d .)"
	QT_INSTALLER="qt-unified-linux-x86_64-${QT_INSTALLER_VERSION}-online.run"
	tMajDotMinVer="$(echo $QT_INSTALLER_VERSION | awk -F. '{print $1"."$2}')"
	verified_download $QT_INSTALLER $QT_INSTALLER_SHA256 \
		"https://master.qt.io/archive/online_installers/${tMajDotMinVer}/${QT_INSTALLER}"

	chmod a+x ${QT_INSTALLER}
	QT_QPA_PLATFORM=minimal ./${QT_INSTALLER} \
		install qt.qt5.${QT_VERSION_CODE}.android$(get_qt_arch _) \
		--accept-licenses --accept-obligations --confirm-command \
		--default-answer --no-default-installations \
		--root "${QT_INSTALL_PATH}"
}

## More information available at retroshare://file?name=Android%20Native%20Development%20Kit%20Cookbook.pdf&size=29214468&hash=0123361c1b14366ce36118e82b90faf7c7b1b136
task_register build_bzlib
build_bzlib()
{
	B_dir="bzip2-${BZIP2_SOURCE_VERSION}"
	rm -rf $B_dir

	verified_download $B_dir.tar.gz $BZIP2_SOURCE_SHA256 \
		http://distfiles.gentoo.org/distfiles/bzip2-${BZIP2_SOURCE_VERSION}.tar.gz

	tar -xf $B_dir.tar.gz
	cd $B_dir
	sed -i "/^CC=.*/d" Makefile
	sed -i "/^AR=.*/d" Makefile
	sed -i "/^RANLIB=.*/d" Makefile
	sed -i "/^LDFLAGS=.*/d" Makefile
	sed -i "s/^all: libbz2.a bzip2 bzip2recover test/all: libbz2.a bzip2 bzip2recover/" Makefile
	make -j${HOST_NUM_CPU}
	make install PREFIX=${PREFIX}
#	sed -i "/^CC=.*/d" Makefile-libbz2_so
#	make -f Makefile-libbz2_so -j${HOST_NUM_CPU}
#	cp libbz2.so.1.0.6 ${SYSROOT}/usr/lib/libbz2.so
	cd ..
}

## More information available at http://doc.qt.io/qt-5/opensslsupport.html
task_register build_openssl
build_openssl()
{
	B_dir="openssl-${OPENSSL_SOURCE_VERSION}"
	rm -rf $B_dir

	verified_download $B_dir.tar.gz $OPENSSL_SOURCE_SHA256 \
		https://www.openssl.org/source/$B_dir.tar.gz

	tar -xf $B_dir.tar.gz
	cd $B_dir
## We link openssl statically to avoid android silently sneaking in his own
## version of libssl.so (we noticed this because it had some missing symbol
## that made RS crash), the crash in some android version is only one of the
## possible problems the fact that android insert his own binary libssl.so pose
## non neglegible security concerns.
	oBits="32"
	[[ ${ANDROID_NDK_ARCH} =~ .*64.* ]] && oBits=64

	ANDROID_NDK="${ANDROID_NDK_PATH}" PATH="${SYSROOT}/bin/:${PATH}" \
		./Configure linux-generic${oBits} -fPIC --prefix="${PREFIX}" \
		--openssldir="${SYSROOT}/etc/ssl"
#	sed -i 's/LIBNAME=$$i LIBVERSION=$(SHLIB_MAJOR).$(SHLIB_MINOR) \\/LIBNAME=$$i \\/g' Makefile
#	sed -i '/LIBCOMPATVERSIONS=";$(SHLIB_VERSION_HISTORY)" \\/d' Makefile

	# Avoid documentation build which is unneded and time consuming
	echo "exit 0; " > util/process_docs.pl
	
	make -j${HOST_NUM_CPU}
	make install
	rm -f ${PREFIX}/lib/libssl.so*
	rm -f ${PREFIX}/lib/libcrypto.so*
	cd ..
}

task_register build_sqlite
build_sqlite()
{
	B_dir="sqlite-autoconf-${SQLITE_SOURCE_VERSION}"
	rm -rf $B_dir

	verified_download $B_dir.tar.gz $SQLITE_SOURCE_SHA256 \
		https://www.sqlite.org/${SQLITE_SOURCE_YEAR}/$B_dir.tar.gz

	tar -xf $B_dir.tar.gz
	cd $B_dir
	./configure --with-pic --prefix="${PREFIX}" --host=${cArch}-linux
	make -j${HOST_NUM_CPU}
	make install
	rm -f ${PREFIX}/lib/libsqlite3.so*
	cd ..
}

task_register build_sqlcipher
build_sqlcipher()
{
	task_run build_sqlite

	B_dir="sqlcipher-${SQLCIPHER_SOURCE_VERSION}"
	rm -rf $B_dir

	T_file="${B_dir}.tar.gz"

	verified_download $T_file $SQLCIPHER_SOURCE_SHA256 \
		https://github.com/sqlcipher/sqlcipher/archive/v${SQLCIPHER_SOURCE_VERSION}.tar.gz

	tar -xf $T_file
	cd $B_dir
#	case "${ANDROID_NDK_ARCH}" in
#	"arm64")
#	# SQLCipher config.sub is outdated and doesn't recognize newer architectures
#		rm config.sub
#		autoreconf --verbose --install --force
#		automake --add-missing --copy --force-missing
#	;;
#	esac
	./configure --with-pic --build=$(sh ./config.guess) \
		--host=${cArch}-linux \
		--prefix="${PREFIX}" --with-sysroot="${SYSROOT}" \
		--enable-tempstore=yes \
		--disable-tcl --disable-shared \
		CFLAGS="-DSQLITE_HAS_CODEC" LDFLAGS="${PREFIX}/lib/libcrypto.a"
	make -j${HOST_NUM_CPU}
	make install
	cd ..
}

task_register build_libupnp
build_libupnp()
{
	B_dir="pupnp-release-${LIBUPNP_SOURCE_VERSION}"
	B_ext=".tar.gz"
	B_file="${B_dir}${B_ext}"
	rm -rf $B_dir

	verified_download $B_file $LIBUPNP_SOURCE_SHA256 \
		https://github.com/mrjimenez/pupnp/archive/release-${LIBUPNP_SOURCE_VERSION}${B_ext}

	tar -xf $B_file
	cd $B_dir
	./bootstrap
## liupnp must be configured as static library because if not the linker will
## look for libthreadutils.so.6 at runtime that cannot be packaged on android
## as it supports only libname.so format for libraries, thus resulting in a
## crash at startup.
	./configure --with-pic --enable-static --disable-shared --disable-samples \
		--disable-largefile \
		--prefix="${PREFIX}" --host=${cArch}-linux
	make -j${HOST_NUM_CPU}
	make install
	cd ..
}

task_register build_rapidjson
build_rapidjson()
{
	B_dir="rapidjson-${RAPIDJSON_SOURCE_VERSION}"
	D_file="${B_dir}.tar.gz"
	verified_download $D_file $RAPIDJSON_SOURCE_SHA256 \
		https://github.com/Tencent/rapidjson/archive/v${RAPIDJSON_SOURCE_VERSION}.tar.gz
	tar -xf $D_file
	cp -r "${B_dir}/include/rapidjson/" "${PREFIX}/include/rapidjson"
}

task_register build_restbed
build_restbed()
{
	S_dir="restbed"
	B_dir="${S_dir}-build"
	git_source_get "$S_dir" "$RESTBED_SOURCE_REPO" "${RESTBED_SOURCE_VERSION}" \
		"dependency/asio" "dependency/catch"

	rm -rf "$B_dir"; mkdir "$B_dir"
	pushd "$B_dir"
	andro_cmake -DBUILD_TESTS=OFF -DBUILD_SSL=OFF -B. -H../${S_dir}
	make -j${HOST_NUM_CPU}
	make install
	popd
}

task_register build_udp-discovery-cpp
build_udp-discovery-cpp()
{
	S_dir="udp-discovery-cpp"
	[ -d $S_dir ] || git clone $UDP_DISCOVERY_CPP_SOURCE $S_dir
	cd $S_dir
	git checkout $UDP_DISCOVERY_CPP_VERSION
	cd ..

	B_dir="udp-discovery-cpp-build"
	rm -rf ${B_dir}; mkdir ${B_dir}; cd ${B_dir}
	andro_cmake -B. -H../$S_dir
	make -j${HOST_NUM_CPU}
	cp libudp-discovery.a "${PREFIX}/lib/"
	cp ../$S_dir/*.hpp "${PREFIX}/include/"
	cd ..
}

task_register build_xapian
build_xapian()
{
	B_dir="xapian-core-${XAPIAN_SOURCE_VERSION}"
	D_file="$B_dir.tar.xz"
	verified_download $D_file $XAPIAN_SOURCE_SHA256 \
		https://oligarchy.co.uk/xapian/${XAPIAN_SOURCE_VERSION}/$D_file
	rm -rf $B_dir
	tar -xf $D_file
	cd $B_dir
	B_endiannes_detection_failure_workaround="ac_cv_c_bigendian=no"
	B_large_file=""
	[ "${ANDROID_PLATFORM_VER}" -lt "24" ] && B_large_file="--disable-largefile"
	./configure ${B_endiannes_detection_failure_workaround} ${B_large_file} \
		--with-pic \
		--disable-backend-inmemory --disable-backend-remote \
		--disable--backend-chert --enable-backend-glass \
		--host=${cArch}-linux --enable-static --disable-shared \
		--prefix="${PREFIX}" --with-sysroot="${SYSROOT}"
	make -j${HOST_NUM_CPU}
	make install
	cd ..
}

task_register build_miniupnpc
build_miniupnpc()
{
	S_dir="miniupnpc-${MINIUPNPC_SOURCE_VERSION}"
	B_dir="miniupnpc-${MINIUPNPC_SOURCE_VERSION}-build"
	D_file="$S_dir.tar.gz"
	verified_download $D_file $MINIUPNPC_SOURCE_SHA256 \
		http://miniupnp.free.fr/files/${D_file}
	rm -rf $S_dir $B_dir
	tar -xf $D_file
	mkdir $B_dir
	cd $B_dir
	andro_cmake \
		-DUPNPC_BUILD_STATIC=TRUE \
		-DUPNPC_BUILD_SHARED=FALSE \
		-DUPNPC_BUILD_TESTS=FALSE \
		-DUPNPC_BUILD_SAMPLE=FALSE \
		-B. -S../$S_dir
	make -j${HOST_NUM_CPU}
	make install
	cd ..
}

task_register build_zlib
build_zlib()
{
	S_dir="zlib-${ZLIB_SOURCE_VERSION}"
	B_dir="zlib-${ZLIB_SOURCE_VERSION}-build"
	D_file="$S_dir.tar.xz"
	verified_download $D_file $ZLIB_SOURCE_SHA256 \
		http://www.zlib.net/${D_file}
	rm -rf $S_dir $B_dir
	tar -xf $D_file
	mkdir $B_dir
	cd $B_dir
	andro_cmake -B. -S../$S_dir
	make -j${HOST_NUM_CPU}
	make install
	rm -f ${PREFIX}/lib/libz.so*
	cd ..
}

task_register build_libpng
build_libpng()
{
	task_run build_zlib

	S_dir="libpng-${LIBPNG_SOURCE_VERSION}"
	B_dir="libpng-${LIBPNG_SOURCE_VERSION}-build"
	D_file="$S_dir.tar.xz"
	verified_download $D_file $LIBPNG_SOURCE_SHA256 \
		https://download.sourceforge.net/libpng/${D_file}
	rm -rf $S_dir $B_dir
	tar -xf $D_file

	# libm is part of bionic An android
	sed -i -e 's/find_library(M_LIBRARY m)/set(M_LIBRARY "")/' $S_dir/CMakeLists.txt
	
	# Disable hardware acceleration as they are problematic for Android
	# compilation and are not supported by all phones, it is necessary to fiddle
	# with CMakeLists.txt as libpng 1.6.37 passing it as cmake options seems not
	# working properly
	# https://github.com/imagemin/optipng-bin/issues/97
	# https://github.com/opencv/opencv/issues/7600
	echo "add_definitions(-DPNG_ARM_NEON_OPT=0)" >> $S_dir/CMakeLists.txt

	mkdir $B_dir
	pushd $B_dir

	HW_OPT="OFF"
#	[ "$ANDROID_PLATFORM_VER" -ge "22" ] && HW_OPT="ON"

	andro_cmake \
		-DPNG_SHARED=OFF \
		-DPNG_STATIC=ON \
		-DPNG_TESTS=OFF \
		-DPNG_HARDWARE_OPTIMIZATIONS=$HW_OPT \
		-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
		-B. -S../$S_dir
	make -j${HOST_NUM_CPU}
	make install
	popd
}

task_register build_libjpeg
build_libjpeg()
{
	S_dir="jpeg-${LIBJPEG_SOURCE_VERSION}"
	D_file="jpegsrc.v${LIBJPEG_SOURCE_VERSION}.tar.gz"
	verified_download $D_file $LIBJPEG_SOURCE_SHA256 \
		https://www.ijg.org/files/$D_file
	rm -rf $S_dir
	tar -xf $D_file
	cd $S_dir
	./configure --with-pic --prefix="${PREFIX}" --host=${cArch}-linux
	make -j${HOST_NUM_CPU}
	make install
	rm -f ${PREFIX}/lib/libjpeg.so*
	cd ..
}

task_register build_tiff
build_tiff()
{
	S_dir="tiff-${TIFF_SOURCE_VERSION}"
	B_dir="${S_dir}-build"
	D_file="tiff-${TIFF_SOURCE_VERSION}.tar.gz"

	verified_download $D_file $TIFF_SOURCE_SHA256 \
		https://download.osgeo.org/libtiff/${D_file}

	rm -rf $S_dir $B_dir
	tar -xf $D_file
	mkdir $B_dir

	# Disable tools building, not needed for retroshare, and depending on some
	# OpenGL headers not available on Android
	echo "" > $S_dir/tools/CMakeLists.txt

	# Disable tests building, not needed for retroshare, and causing linker
	# errors
	echo "" > $S_dir/test/CMakeLists.txt
	
	# Disable extra tools building, not needed for retroshare, and causing
	# linker errors
	echo "" > $S_dir/contrib/CMakeLists.txt
	
	# Disable more unneded stuff 
	echo "" > $S_dir/build/CMakeLists.txt
	echo "" > $S_dir/html/CMakeLists.txt
	echo "" > $S_dir/man/CMakeLists.txt
	echo "" > $S_dir/port/CMakeLists.txt

	# Change to static library build
	sed -i 's\add_library(tiff\add_library(tiff STATIC\' \
		$S_dir/libtiff/CMakeLists.txt

	cd $B_dir
	#TODO: build dependecies to support more formats
	andro_cmake \
		-Dlibdeflate=OFF -Djbig=OFF -Dlzma=OFF -Dzstd=OFF -Dwebp=OFF \
		-Djpeg12=OFF \
		-Dcxx=OFF \
		-B. -S../$S_dir
	make -j${HOST_NUM_CPU}
	make install
	cd ..
}

task_register build_cimg
build_cimg()
{
	task_run build_libpng
	task_run build_libjpeg
	task_run build_tiff

	S_dir="CImg-${CIMG_SOURCE_VERSION}"
	D_file="CImg_${CIMG_SOURCE_VERSION}.zip"

	verified_download $D_file $CIMG_SOURCE_SHA256 \
		https://cimg.eu/files/${D_file}

	unzip -o $D_file

	cp --archive --verbose "$S_dir/CImg.h" "$PREFIX/include/"
}

task_register build_phash
build_phash()
{
	task_run build_cimg

	S_dir="pHash"
	B_dir="${S_dir}-build"

	git_source_get "$S_dir" "$PHASH_SOURCE_REPO" "${PHASH_SOURCE_VERSION}"

	rm -rf $B_dir; mkdir $B_dir ; pushd $B_dir
	andro_cmake -DPHASH_DYNAMIC=OFF -DPHASH_STATIC=ON  -B. -H../pHash
	make -j${HOST_NUM_CPU}
	make install
	popd
}

task_register build_mvptree
build_mvptree()
{
	S_dir="mvptree"
	B_dir="${S_dir}-build"

	git_source_get "$S_dir" "$MVPTREE_SOURCE_REPO" "${MVPTREE_SOURCE_VERSION}"
	rm -rf $B_dir; mkdir $B_dir ; pushd $B_dir
	andro_cmake -B. -H../${S_dir}
	make -j${HOST_NUM_CPU}
	make install
	popd
}

task_register get_native_libs_toolchain_path
get_native_libs_toolchain_path()
{
	echo ${NATIVE_LIBS_TOOLCHAIN_PATH}
}

task_register build_default_toolchain
build_default_toolchain()
{
	task_run bootstrap_toolchain || return $?
	task_run build_bzlib || return $?
	task_run build_openssl || return $?
	task_run build_sqlcipher || return $?
	task_run build_rapidjson || return $?
	task_run build_restbed || return $?
	task_run build_udp-discovery-cpp || return $?
	task_run build_xapian || return $?
	task_run build_miniupnpc || return $?
	task_run build_phash || return $?
	task_run build_mvptree || return $?
	task_run deduplicate_includes || return $?
	task_run get_native_libs_toolchain_path || return $?
}

if [ "$1" == "" ]; then
	rm -rf "$REPORT_DIR"
	mkdir -p "$REPORT_DIR"
	cat "$0" > "$REPORT_DIR/build_script"
	env > "$REPORT_DIR/build_env"
	build_default_toolchain
else
	# do not delete report directory in this case so we can reuse material
	# produced by previous run, like deduplicated includes
	mkdir -p "$REPORT_DIR"
	while [ "$1" != "" ] ; do
		task_zap $1
		task_run $1 || exit $?
		shift
	done
fi
