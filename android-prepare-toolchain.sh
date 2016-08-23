#!/bin/bash

## You are supposed to provide the following variables according to your system setup
[ -z ${ANDROID_NDK_PATH+x} ] && export ANDROID_NDK_PATH="/opt/android-ndk/"
[ -z ${ANDROID_NDK_ARCH+x} ] && export ANDROID_NDK_ARCH="arm"
[ -z ${ANDROID_NDK_ABI_VER+x} ] && export ANDROID_NDK_ABI_VER="4.9"
[ -z ${ANDROID_PLATFORM_VER+x} ] && export ANDROID_PLATFORM_VER="18"
[ -z ${NDK_TOOLCHAIN_PATH+x} ] && export NDK_TOOLCHAIN_PATH="/home/$(whoami)/Development/android-toolchains/retroshare-android-${ANDROID_PLATFORM_VER}-${ANDROID_NDK_ARCH}-abi${ANDROID_NDK_ABI_VER}/"
[ -z ${HOST_NUM_CPU+x} ] && export HOST_NUM_CPU=4

runDir="$(pwd)"

## You should not edit the following variables
if [ "${ANDROID_NDK_ARCH}" == "x86" ]; then
	cArch="i686"
	eABI=""
else
	cArch="${ANDROID_NDK_ARCH}"
	eABI="eabi"
fi
export SYSROOT="${NDK_TOOLCHAIN_PATH}/sysroot"
export PREFIX="${SYSROOT}"
export CC="${NDK_TOOLCHAIN_PATH}/bin/${cArch}-linux-android${eABI}-gcc"
export CXX="${NDK_TOOLCHAIN_PATH}/bin/${cArch}-linux-android${eABI}-g++"
export AR="${NDK_TOOLCHAIN_PATH}/bin/${cArch}-linux-android${eABI}-ar"
export RANLIB="${NDK_TOOLCHAIN_PATH}/bin/${cArch}-linux-android${eABI}-ranlib"
export ANDROID_DEV="${ANDROID_NDK_PATH}/platforms/android-${ANDROID_PLATFORM_VER}/arch-${ANDROID_NDK_ARCH}/usr"


## More information available at https://android.googlesource.com/platform/ndk/+/ics-mr0/docs/STANDALONE-TOOLCHAIN.html
build_toolchain()
{
	rm -rf ${NDK_TOOLCHAIN_PATH}
	[ "${ANDROID_NDK_ARCH}" == "x86" ] && toolchainName="${ANDROID_NDK_ARCH}-${ANDROID_NDK_ABI_VER}" || toolchainName="${ANDROID_NDK_ARCH}-linux-androideabi-${ANDROID_NDK_ABI_VER}" 
	${ANDROID_NDK_PATH}/build/tools/make-standalone-toolchain.sh --ndk-dir=${ANDROID_NDK_PATH} --arch=${ANDROID_NDK_ARCH} --install-dir=${NDK_TOOLCHAIN_PATH} --platform=android-${ANDROID_PLATFORM_VER} --toolchain=${toolchainName} --verbose
}

## More information available at retroshare://file?name=Android%20Native%20Development%20Kit%20Cookbook.pdf&size=29214468&hash=0123361c1b14366ce36118e82b90faf7c7b1b136
build_bzlib()
{
	B_dir="bzip2-1.0.6"
	rm -rf $B_dir
	[ -f $B_dir.tar.gz ] || wget http://www.bzip.org/1.0.6/bzip2-1.0.6.tar.gz
	tar -xf $B_dir.tar.gz
	cd $B_dir
	sed -i "/^CC=.*/d" Makefile
	sed -i "/^AR=.*/d" Makefile
	sed -i "/^RANLIB=.*/d" Makefile
	sed -i "/^LDFLAGS=.*/d" Makefile
	sed -i "s/^all: libbz2.a bzip2 bzip2recover test/all: libbz2.a bzip2 bzip2recover/" Makefile
	make -j${HOST_NUM_CPU}
	make install PREFIX=${SYSROOT}/usr
#	sed -i "/^CC=.*/d" Makefile-libbz2_so
#	make -f Makefile-libbz2_so -j${HOST_NUM_CPU}
#	cp libbz2.so.1.0.6 ${SYSROOT}/usr/lib/libbz2.so
	cd ..
}

## More information available at http://doc.qt.io/qt-5/opensslsupport.html
build_openssl()
{
	B_dir="openssl-1.0.2h"
	rm -rf $B_dir
	[ -f $B_dir.tar.gz ] || wget https://www.openssl.org/source/$B_dir.tar.gz
	tar -xf $B_dir.tar.gz
	cd $B_dir
	if [ "${ANDROID_NDK_ARCH}" == "arm" ]; then
		oArch="armv7"
	else
		oArch="${ANDROID_NDK_ARCH}"
	fi
#	ANDROID_NDK_ROOT="${ANDROID_NDK_PATH}" ./Configure android-${oArch} shared --prefix="${SYSROOT}/usr" --openssldir="${SYSROOT}/etc/ssl"
## We link openssl statically to avoid android silently sneaking in his own
## version of libssl.so (we noticed this because it had some missing symbol
## that made RS crash), the crash in some android version is only one of the
## possible problems the fact that android insert his own binary libssl.so pose
## non neglegible security concerns.
	ANDROID_NDK_ROOT="${ANDROID_NDK_PATH}" ./Configure android-${oArch} --prefix="${SYSROOT}/usr" --openssldir="${SYSROOT}/etc/ssl"
	sed -i 's/LIBNAME=$$i LIBVERSION=$(SHLIB_MAJOR).$(SHLIB_MINOR) \\/LIBNAME=$$i \\/g' Makefile
	sed -i '/LIBCOMPATVERSIONS=";$(SHLIB_VERSION_HISTORY)" \\/d' Makefile
	make -j${HOST_NUM_CPU}
	make install
#	cp *.so "${SYSROOT}/usr/lib"
	cd ..
}

build_sqlite()
{
	B_dir="sqlite-autoconf-3130000"
	[ -f $B_dir.tar.gz ] || wget https://www.sqlite.org/2016/$B_dir.tar.gz
	tar -xf $B_dir.tar.gz
	cd $B_dir
	./configure --prefix="${SYSROOT}/usr" --host=${ANDROID_NDK_ARCH}-linux
	make -j${HOST_NUM_CPU}
	make install
	rm -f ${SYSROOT}/usr/lib/libsqlite3.so*
	${CC} -shared -o libsqlite3.so -fPIC sqlite3.o -ldl
	cp libsqlite3.so "${SYSROOT}/usr/lib"
	cd ..
}

build_sqlcipher()
{
	echo "sqlcipher not supported yet on android"
	return 0

	cd sqlcipher
	./configure --enable-tempstore=yes CFLAGS="-DSQLITE_HAS_CODEC" LDFLAGS="${SYSROOT}/usr/lib/libcrypto.a"
	make -j${HOST_NUM_CPU}
	make install
	cd ..
}

build_libupnp()
{
	B_dir="libupnp-1.6.20"
	rm -rf $B_dir
	[ -f $B_dir.tar.bz2 ] || wget https://sourceforge.net/projects/pupnp/files/pupnp/libUPnP%201.6.20/$B_dir.tar.bz2
	tar -xf $B_dir.tar.bz2
	cd $B_dir
## liupnp must be configured as static library because if not the linker will
## look for libthreadutils.so.6 at runtime that cannot be packaged on android
## as it supports only libname.so format for libraries, thus resulting in a
## crash at startup.
	./configure --enable-static --disable-shared --disable-samples --prefix="${SYSROOT}/usr" --host=${ANDROID_NDK_ARCH}-linux
	make -j${HOST_NUM_CPU}
	make install
	cd ..
}

build_libmicrohttpd()
{
	echo "libmicrohttpd not supported yet on android"
	return 0

	B_dir="libmicrohttpd-0.9.50"
	rm -rf $B_dir
	[ -f $B_dir.tar.gz ] || wget ftp://ftp.gnu.org/gnu/libmicrohttpd/$B_dir.tar.gz
	tar -xf $B_dir.tar.gz
	cd $B_dir
	./configure --prefix="${SYSROOT}/usr" --host=${ANDROID_NDK_ARCH}-linux
	#make -e ?
	make -j${HOST_NUM_CPU}
	make install
	cd ..
}

build_toolchain
build_bzlib
build_openssl
build_sqlite
build_libupnp
