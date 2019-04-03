#!/bin/bash

## Define default value for variable, take two arguments, $1 variable name,
## $2 default variable value, if the variable is not already define define it
## with default value.
function define_default_value()
{
	VAR_NAME="${1}"
	DEFAULT_VALUE="${2}"

	[ -z "${!VAR_NAME}" ] && export ${VAR_NAME}="${DEFAULT_VALUE}"
}

## You are supposed to provide the following variables according to your system setup
define_default_value ANDROID_NDK_PATH "/opt/android-ndk/"
define_default_value ANDROID_NDK_ARCH "arm"
define_default_value ANDROID_PLATFORM_VER "16"
define_default_value NATIVE_LIBS_TOOLCHAIN_PATH "${HOME}/Builds/android-toolchains/retroshare-android-${ANDROID_PLATFORM_VER}-${ANDROID_NDK_ARCH}/"
define_default_value HOST_NUM_CPU $(nproc)

define_default_value BZIP2_SOURCE_VERSION "1.0.6"
define_default_value BZIP2_SOURCE_SHA256 a2848f34fcd5d6cf47def00461fcb528a0484d8edef8208d6d2e2909dc61d9cd

define_default_value OPENSSL_SOURCE_VERSION "1.1.1"
define_default_value OPENSSL_SOURCE_SHA256 2836875a0f89c03d0fdf483941512613a50cfb421d6fd94b9f41d7279d586a3d

define_default_value SQLITE_SOURCE_YEAR "2018"
define_default_value SQLITE_SOURCE_VERSION "3250200"
define_default_value SQLITE_SOURCE_SHA256 da9a1484423d524d3ac793af518cdf870c8255d209e369bd6a193e9f9d0e3181

define_default_value SQLCIPHER_SOURCE_VERSION "3.4.2"
define_default_value SQLCIPHER_SOURCE_SHA256 69897a5167f34e8a84c7069f1b283aba88cdfa8ec183165c4a5da2c816cfaadb

define_default_value LIBUPNP_SOURCE_VERSION "1.6.25"
define_default_value LIBUPNP_SOURCE_SHA256 c5a300b86775435c076d58a79cc0d5a977d76027d2a7d721590729b7f369fa43

define_default_value INSTALL_QT_ANDROID "false"
define_default_value QT_VERSION "5.12.0"
define_default_value QT_ANDROID_INSTALLER_SHA256 a214084e2295c9a9f8727e8a0131c37255bf724bfc69e80f7012ba3abeb1f763

define_default_value RESTBED_SOURCE_VERSION "4.6"

define_default_value XAPIAN_SOURCE_VERSION "1.4.7"
define_default_value XAPIAN_SOURCE_SHA256 13f08a0b649c7afa804fa0e85678d693fd6069dd394c9b9e7d41973d74a3b5d3

define_default_value RAPIDJSON_SOURCE_VERSION "1.1.0"
define_default_value RAPIDJSON_SOURCE_SHA256 bf7ced29704a1e696fbccf2a2b4ea068e7774fa37f6d7dd4039d0787f8bed98e


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



if [ "${ANDROID_NDK_ARCH}" == "x86" ]; then
	cArch="i686"
	eABI=""
else
	cArch="${ANDROID_NDK_ARCH}"
	eABI="eabi"
fi
export SYSROOT="${NATIVE_LIBS_TOOLCHAIN_PATH}/sysroot/"
export PREFIX="${SYSROOT}/usr/"
export CC="${NATIVE_LIBS_TOOLCHAIN_PATH}/bin/${cArch}-linux-android${eABI}-clang"
export CXX="${NATIVE_LIBS_TOOLCHAIN_PATH}/bin/${cArch}-linux-android${eABI}-clang++"
export AR="${NATIVE_LIBS_TOOLCHAIN_PATH}/bin/${cArch}-linux-android${eABI}-ar"
export RANLIB="${NATIVE_LIBS_TOOLCHAIN_PATH}/bin/${cArch}-linux-android${eABI}-ranlib"


## More information available at https://android.googlesource.com/platform/ndk/+/ics-mr0/docs/STANDALONE-TOOLCHAIN.html
build_toolchain()
{
	rm -rf ${NATIVE_LIBS_TOOLCHAIN_PATH}
	${ANDROID_NDK_PATH}/build/tools/make_standalone_toolchain.py --verbose \
		--arch ${ANDROID_NDK_ARCH} --install-dir ${NATIVE_LIBS_TOOLCHAIN_PATH} \
		--api ${ANDROID_PLATFORM_VER}
	find "${PREFIX}/include/" -not -type d > "${NATIVE_LIBS_TOOLCHAIN_PATH}/deletefiles"
}

## This avoid <cmath> include errors due to -isystem and -I ordering issue
delete_copied_includes()
{
	cat "${NATIVE_LIBS_TOOLCHAIN_PATH}/deletefiles" | while read delFile ; do
		rm "$delFile"
	done
}

## More information available at https://gitlab.com/relan/provisioners/merge_requests/1 and http://stackoverflow.com/a/34032216
install_qt_android()
{
	QT_VERSION_CODE=$(echo $QT_VERSION | tr -d .)
	QT_INSTALL_PATH=${NATIVE_LIBS_TOOLCHAIN_PATH}/Qt
	QT_INSTALLER="qt-unified-linux-x64-3.0.2-online.run"

	verified_download $QT_INSTALLER $QT_ANDROID_INSTALLER_SHA256 \
		http://master.qt.io/archive/online_installers/3.0/${QT_INSTALLER}

	chmod a+x ${QT_INSTALLER}

	QT_INSTALLER_SCRIPT="qt_installer_script.js"
	cat << EOF > "${QT_INSTALLER_SCRIPT}"
function Controller() {
    installer.autoRejectMessageBoxes();
    installer.installationFinished.connect(function() {
        gui.clickButton(buttons.NextButton);
    });

    var welcomePage = gui.pageWidgetByObjectName("WelcomePage");
    welcomePage.completeChanged.connect(function() {
        if (gui.currentPageWidget().objectName == welcomePage.objectName)
            gui.clickButton(buttons.NextButton);
    });
}

Controller.prototype.WelcomePageCallback = function() {
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.CredentialsPageCallback = function() {
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.IntroductionPageCallback = function() {
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.TargetDirectoryPageCallback = function() {
    gui.currentPageWidget().TargetDirectoryLineEdit.setText("$QT_INSTALL_PATH");
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.ComponentSelectionPageCallback = function() {
    var widget = gui.currentPageWidget();

    // You can get these component names by running the installer with the
    // --verbose flag. It will then print out a resource tree.

    widget.deselectComponent("qt.tools.qtcreator");
    widget.deselectComponent("qt.tools.doc");
    widget.deselectComponent("qt.tools.examples");

    widget.selectComponent("qt.$QT_VERSION_CODE.android_armv7");

    gui.clickButton(buttons.NextButton);
}

Controller.prototype.LicenseAgreementPageCallback = function() {
    gui.currentPageWidget().AcceptLicenseRadioButton.setChecked(true);
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.StartMenuDirectoryPageCallback = function() {
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.ReadyForInstallationPageCallback = function() {
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.FinishedPageCallback = function() {
    var checkBoxForm = gui.currentPageWidget().LaunchQtCreatorCheckBoxForm;
    if (checkBoxForm && checkBoxForm.launchQtCreatorCheckBox)
        checkBoxForm.launchQtCreatorCheckBox.checked = false;
    gui.clickButton(buttons.FinishButton);
}
EOF

QT_QPA_PLATFORM=minimal ./${QT_INSTALLER} --script ${QT_INSTALLER_SCRIPT}
}

## More information available at retroshare://file?name=Android%20Native%20Development%20Kit%20Cookbook.pdf&size=29214468&hash=0123361c1b14366ce36118e82b90faf7c7b1b136
build_bzlib()
{
	B_dir="bzip2-${BZIP2_SOURCE_VERSION}"
	rm -rf $B_dir

	verified_download $B_dir.tar.gz $BZIP2_SOURCE_SHA256 \
		http://trumpetti.atm.tut.fi/gentoo/distfiles/bzip2-${BZIP2_SOURCE_VERSION}.tar.gz

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
		./Configure linux-generic${oBits} --prefix="${PREFIX}" \
		--openssldir="${SYSROOT}/etc/ssl"
#	sed -i 's/LIBNAME=$$i LIBVERSION=$(SHLIB_MAJOR).$(SHLIB_MINOR) \\/LIBNAME=$$i \\/g' Makefile
#	sed -i '/LIBCOMPATVERSIONS=";$(SHLIB_VERSION_HISTORY)" \\/d' Makefile
	make -j${HOST_NUM_CPU}
	make install
	rm -f ${PREFIX}/lib/libssl.so*
	rm -f ${PREFIX}/lib/libcrypto.so*
	cd ..
}

build_sqlite()
{
	B_dir="sqlite-autoconf-${SQLITE_SOURCE_VERSION}"

	verified_download $B_dir.tar.gz $SQLITE_SOURCE_SHA256 \
		https://www.sqlite.org/${SQLITE_SOURCE_YEAR}/$B_dir.tar.gz

	tar -xf $B_dir.tar.gz
	cd $B_dir
	./configure --prefix="${PREFIX}" --host=${ANDROID_NDK_ARCH}-linux
	make -j${HOST_NUM_CPU}
	make install
	rm -f ${PREFIX}/lib/libsqlite3.so*
#	${CC} -shared -o libsqlite3.so -fPIC sqlite3.o -ldl
#	cp libsqlite3.so "${SYSROOT}/usr/lib"
	cd ..
}

build_sqlcipher()
{
	B_dir="sqlcipher-${SQLCIPHER_SOURCE_VERSION}"
	rm -rf $B_dir

	T_file="${B_dir}.tar.gz"

	verified_download $T_file $SQLCIPHER_SOURCE_SHA256 \
		https://github.com/sqlcipher/sqlcipher/archive/v${SQLCIPHER_SOURCE_VERSION}.tar.gz

	tar -xf $T_file
	cd $B_dir
	./configure --build=$(sh ./config.guess) \
		--host=${ANDROID_NDK_ARCH}-linux \
		--prefix="${PREFIX}" --with-sysroot="${SYSROOT}" \
		--enable-tempstore=yes \
		--disable-tcl --disable-shared \
		CFLAGS="-DSQLITE_HAS_CODEC" LDFLAGS="${PREFIX}/lib/libcrypto.a"
	make -j${HOST_NUM_CPU}
	make install
	cd ..
}

build_libupnp()
{
	B_dir="libupnp-${LIBUPNP_SOURCE_VERSION}"
	rm -rf $B_dir

	verified_download $B_dir.tar.bz2 $LIBUPNP_SOURCE_SHA256 \
		https://sourceforge.net/projects/pupnp/files/pupnp/libUPnP%20${LIBUPNP_SOURCE_VERSION}/$B_dir.tar.bz2

	tar -xf $B_dir.tar.bz2
	cd $B_dir
## liupnp must be configured as static library because if not the linker will
## look for libthreadutils.so.6 at runtime that cannot be packaged on android
## as it supports only libname.so format for libraries, thus resulting in a
## crash at startup.
	./configure --enable-static --disable-shared --disable-samples \
		--prefix="${PREFIX}" --host=${ANDROID_NDK_ARCH}-linux
	make -j${HOST_NUM_CPU}
	make install
	cd ..
}

build_rapidjson()
{
	B_dir="rapidjson-${RAPIDJSON_SOURCE_VERSION}"
	D_file="${B_dir}.tar.gz"
	verified_download $D_file $RAPIDJSON_SOURCE_SHA256 \
		https://github.com/Tencent/rapidjson/archive/v${RAPIDJSON_SOURCE_VERSION}.tar.gz
	tar -xf $D_file
	cp -r "${B_dir}/include/rapidjson/" "${PREFIX}/include/rapidjson"
}

build_restbed()
{
	[ -d restbed ] || git clone --depth=2000 https://github.com/Corvusoft/restbed.git
	cd restbed
	git fetch --tags
	git checkout tags/${RESTBED_SOURCE_VERSION}
	git submodule update --init dependency/asio
	git submodule update --init dependency/catch
	git submodule update --init dependency/kashmir
	cd ..

	rm -rf restbed-build; mkdir restbed-build ; cd restbed-build
	cmake -DBUILD_SSL=OFF -DCMAKE_INSTALL_PREFIX="${PREFIX}" -B. -H../restbed
	make -j${HOST_NUM_CPU}
	make install
	cp "${PREFIX}/library/librestbed.a" "${PREFIX}/lib/"
	cd ..
}

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
		--disable-backend-inmemory --disable-backend-remote \
		--disable--backend-chert --enable-backend-glass \
		--host=${ANDROID_NDK_ARCH}-linux --enable-static --disable-shared \
		--prefix="${PREFIX}" --with-sysroot="${SYSROOT}"
	make -j${HOST_NUM_CPU}
	make install
}

build_toolchain
[ "${INSTALL_QT_ANDROID}X" != "trueX" ] || install_qt_android
build_bzlib
build_openssl
build_sqlite
build_sqlcipher
build_libupnp
build_rapidjson
build_restbed
build_xapian
delete_copied_includes

echo NATIVE_LIBS_TOOLCHAIN_PATH=${NATIVE_LIBS_TOOLCHAIN_PATH}
