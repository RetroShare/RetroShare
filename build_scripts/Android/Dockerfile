## To prepare an image suitable as base for Gitlab CI use
## image name must match gitlab repository name, you can play just with the tag
## the part after :
# export CI_IMAGE_NAME="registry.gitlab.com/retroshare/retroshare:android_arm_base"
# docker build --squash -t "${CI_REGISTRY_IMAGE}" \
# --build-arg QT_INSTALLER_JWT_TOKEN="your qt JWT token goes here"  .
#
# To build Android ARMv8 (64 bit) package pass also
# export CI_IMAGE_NAME="registry.gitlab.com/retroshare/retroshare:android_arm64_base"
# --build-arg ANDROID_NDK_ARCH=arm64 --build-arg ANDROID_PLATFORM_VER=21

## --squash is very important in case of GitlabCI shared runners as they are
## limited to 25GB disk size

## To push it to gitlab CI registry you need first to login and the to push
# docker login registry.gitlab.com
# docker push ${CI_IMAGE_NAME}


FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get clean
RUN apt-get install -y -qq \
	bash build-essential bzip2 cmake curl chrpath doxygen \
	git p7zip python qt5-default qttools5-dev tclsh unzip wget zip

# Dependencies to create Android pkg
RUN apt-get install -y -qq \
	openjdk-8-jre openjdk-8-jdk openjdk-8-jdk-headless gradle

ARG FRESHCLONE=0
ARG REPO_URL=https://gitlab.com/RetroShare/RetroShare.git
ARG REPO_BRANCH=master
ARG REPO_DEPTH="--depth 2000"

RUN git clone $REPO_DEPTH $REPO_URL -b $REPO_BRANCH && cd RetroShare && \
	git fetch --tags

ENV PREPARE_TOOLCHAIN="/RetroShare/build_scripts/Android/prepare-toolchain-clang.sh"
ENV NATIVE_LIBS_TOOLCHAIN_PATH="/android-toolchain/"

ARG ANDROID_PLATFORM_VER=16
ARG ANDROID_NDK_ARCH=arm

ENV ANDROID_SDK_PATH="/opt/android-sdk"
ENV ANDROID_HOME="$ANDROID_SDK_PATH"
ENV ANDROID_SDK_ROOT="$ANDROID_SDK_PATH"

ENV ANDROID_NDK_PATH="/opt/android-ndk"
ENV ANDROID_NDK_ROOT="$ANDROID_NDK_PATH"

ENV PATH="$PATH:$ANDROID_HOME/tools"
ENV PATH="$PATH:$ANDROID_HOME/platform-tools"
ENV JAVA_HOME="/usr/lib/jvm/java-8-openjdk-amd64/"

RUN mkdir /bCache
WORKDIR /bCache

## Quick HACK to ease development
#COPY prepare-toolchain-clang.sh $PREPARE_TOOLCHAIN

RUN $PREPARE_TOOLCHAIN install_android_sdk
RUN $PREPARE_TOOLCHAIN install_android_ndk
RUN $PREPARE_TOOLCHAIN

ARG QT_INSTALLER_JWT_TOKEN
RUN $PREPARE_TOOLCHAIN install_qt_android
# Avoid Qt account details leak into the image
RUN rm -f /root/.local/share/Qt/qtaccount.ini
# Shrink image by removing unneded Qt components
RUN rm -r \
	$NATIVE_LIBS_TOOLCHAIN_PATH/Qt/Docs/ \
	$NATIVE_LIBS_TOOLCHAIN_PATH/Qt/Examples/ \
	$NATIVE_LIBS_TOOLCHAIN_PATH/Qt/Tools/ 

RUN mkdir /jsonapi-generator-build
WORKDIR /jsonapi-generator-build/
RUN qmake ../RetroShare/jsonapi-generator/src/ \
		CONFIG+=no_retroshare_plugins \
		CONFIG+=no_retroshare_service CONFIG+=no_retroshare_gui \
		CONFIG+=rs_jsonapi && \
	make -j$(nproc)

RUN mkdir /retroshare-service-android-build
WORKDIR /retroshare-service-android-build
RUN $($PREPARE_TOOLCHAIN get_qt_dir | head -n 1)/bin/qmake ../RetroShare \
	-spec android-clang \
	CONFIG+=retroshare_service CONFIG+=rs_jsonapi \
	RS_UPNP_LIB=miniupnpc \
	JSONAPI_GENERATOR_EXE=/jsonapi-generator-build/jsonapi-generator \
	NATIVE_LIBS_TOOLCHAIN_PATH=$NATIVE_LIBS_TOOLCHAIN_PATH \
	CONFIG+=rs_perceptual_hash \
	CONFIG+=no_retroshare_gui CONFIG+=no_rs_service_webui_terminal_password \
	CONFIG+=no_rs_service_terminal_login
RUN	make -j$(nproc)
RUN make install INSTALL_ROOT=/retroshare-service-android-build/android-build/
RUN $($PREPARE_TOOLCHAIN get_qt_dir | head -n 1)/bin/androiddeployqt \
	--input retroshare-service/src/android-libretroshare-service.so-deployment-settings.json \
	--output android-build --android-platform android-$ANDROID_PLATFORM_VER \
	--jdk $JAVA_HOME --gradle


RUN rm -rf /bCache

# Clean apt cache
RUN apt-get clean && rm -rf /var/lib/apt/lists/*
