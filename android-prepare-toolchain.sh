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
define_default_value ANDROID_NDK_ABI_VER "4.9"
define_default_value ANDROID_PLATFORM_VER "18"
define_default_value NATIVE_LIBS_TOOLCHAIN_PATH "${HOME}/Builds/android-toolchains/retroshare-android-${ANDROID_PLATFORM_VER}-${ANDROID_NDK_ARCH}-abi${ANDROID_NDK_ABI_VER}/"
define_default_value HOST_NUM_CPU $(nproc)

define_default_value BZIP2_SOURCE_VERSION "1.0.6"
define_default_value BZIP2_SOURCE_SHA256 a2848f34fcd5d6cf47def00461fcb528a0484d8edef8208d6d2e2909dc61d9cd

define_default_value OPENSSL_SOURCE_VERSION "1.0.2n"
define_default_value OPENSSL_SOURCE_SHA256 370babb75f278c39e0c50e8c4e7493bc0f18db6867478341a832a982fd15a8fe

define_default_value SQLITE_SOURCE_YEAR "2018"
define_default_value SQLITE_SOURCE_VERSION "3220000"
define_default_value SQLITE_SOURCE_SHA256 2824ab1238b706bc66127320afbdffb096361130e23291f26928a027b885c612

define_default_value SQLCIPHER_SOURCE_VERSION "3.4.2"
define_default_value SQLCIPHER_SOURCE_SHA256 69897a5167f34e8a84c7069f1b283aba88cdfa8ec183165c4a5da2c816cfaadb

define_default_value LIBUPNP_SOURCE_VERSION "1.6.24"
define_default_value LIBUPNP_SOURCE_SHA256 7d83d79af3bb4062e5c3a58bf2e90d2da5b8b99e2b2d57c23b5b6f766288cf96

define_default_value INSTALL_QT_ANDROID "false"
define_default_value QT_VERSION "5.9.4"
define_default_value QT_ANDROID_INSTALLER_SHA256 a214084e2295c9a9f8727e8a0131c37255bf724bfc69e80f7012ba3abeb1f763


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
export SYSROOT="${NATIVE_LIBS_TOOLCHAIN_PATH}/sysroot"
export PREFIX="${SYSROOT}"
export CC="${NATIVE_LIBS_TOOLCHAIN_PATH}/bin/${cArch}-linux-android${eABI}-gcc"
export CXX="${NATIVE_LIBS_TOOLCHAIN_PATH}/bin/${cArch}-linux-android${eABI}-g++"
export AR="${NATIVE_LIBS_TOOLCHAIN_PATH}/bin/${cArch}-linux-android${eABI}-ar"
export RANLIB="${NATIVE_LIBS_TOOLCHAIN_PATH}/bin/${cArch}-linux-android${eABI}-ranlib"
export ANDROID_DEV="${ANDROID_NDK_PATH}/platforms/android-${ANDROID_PLATFORM_VER}/arch-${ANDROID_NDK_ARCH}/usr"


## More information available at https://android.googlesource.com/platform/ndk/+/ics-mr0/docs/STANDALONE-TOOLCHAIN.html
build_toolchain()
{
	rm -rf ${NATIVE_LIBS_TOOLCHAIN_PATH}
	[ "${ANDROID_NDK_ARCH}" == "x86" ] && toolchainName="${ANDROID_NDK_ARCH}-${ANDROID_NDK_ABI_VER}" || toolchainName="${ANDROID_NDK_ARCH}-linux-androideabi-${ANDROID_NDK_ABI_VER}" 
	${ANDROID_NDK_PATH}/build/tools/make-standalone-toolchain.sh --ndk-dir=${ANDROID_NDK_PATH} --arch=${ANDROID_NDK_ARCH} --install-dir=${NATIVE_LIBS_TOOLCHAIN_PATH} --platform=android-${ANDROID_PLATFORM_VER} --toolchain=${toolchainName} --verbose
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
		http://www.bzip.org/${BZIP2_SOURCE_VERSION}/bzip2-${BZIP2_SOURCE_VERSION}.tar.gz

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
	B_dir="openssl-${OPENSSL_SOURCE_VERSION}"
	rm -rf $B_dir

	verified_download $B_dir.tar.gz $OPENSSL_SOURCE_SHA256 \
		https://www.openssl.org/source/$B_dir.tar.gz

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
	B_dir="sqlite-autoconf-${SQLITE_SOURCE_VERSION}"

	verified_download $B_dir.tar.gz $SQLITE_SOURCE_SHA256 \
		https://www.sqlite.org/${SQLITE_SOURCE_YEAR}/$B_dir.tar.gz

	tar -xf $B_dir.tar.gz
	cd $B_dir
	./configure --prefix="${SYSROOT}/usr" --host=${ANDROID_NDK_ARCH}-linux
	make -j${HOST_NUM_CPU}
	make install
	rm -f ${SYSROOT}/usr/lib/libsqlite3.so*
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
		--prefix="${SYSROOT}/usr" --with-sysroot="${SYSROOT}" \
		--enable-tempstore=yes \
		--disable-tcl --disable-shared \
		CFLAGS="-DSQLITE_HAS_CODEC" LDFLAGS="${SYSROOT}/usr/lib/libcrypto.a"
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

build_rapidjson()
{
	B_dir="rapidjson-1.1.0"
	[ -f $B_dir.tar.gz ] || wget -O $B_dir.tar.gz https://github.com/Tencent/rapidjson/archive/v1.1.0.tar.gz
	tar -xf $B_dir.tar.gz
	cp -r rapidjson-1.1.0/include/rapidjson/ "${SYSROOT}/usr/include/rapidjson"
}

build_toolchain
[ "${INSTALL_QT_ANDROID}X" == "trueX" ] && install_qt_android
build_bzlib
build_openssl
build_sqlite
build_sqlcipher
build_libupnp
build_rapidjson

echo NATIVE_LIBS_TOOLCHAIN_PATH=${NATIVE_LIBS_TOOLCHAIN_PATH}
