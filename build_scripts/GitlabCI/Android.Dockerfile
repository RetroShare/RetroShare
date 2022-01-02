ARG ANDROID_NDK_ARCH=arm
FROM registry.gitlab.com/retroshare/retroshare:android_${ANDROID_NDK_ARCH}_base

ENV APT_UNAT="--assume-yes --quiet"

RUN apt-get update $APT_UNAT && apt-get upgrade $APT_UNAT --show-upgraded

ARG REPO_URL=https://gitlab.com/RetroShare/RetroShare.git
ARG REPO_BRANCH=master

WORKDIR /RetroShare
RUN git remote add testing $REPO_URL && \
	git fetch --tags testing $REPO_BRANCH && \
	git reset --hard testing/$REPO_BRANCH && \
	git --no-pager log --max-count 1

RUN rm -rf /jsonapi-generator-build ; mkdir /jsonapi-generator-build
WORKDIR /jsonapi-generator-build/
RUN qmake ../RetroShare/jsonapi-generator/src/ \
		CONFIG+=no_retroshare_plugins \
		CONFIG+=no_retroshare_service CONFIG+=no_retroshare_gui \
		CONFIG+=rs_jsonapi CONFIG+=no_rs_sam3_libsam3 && \
	make -j$(nproc)

RUN rm -rf /retroshare-service-android-build ; mkdir /retroshare-service-android-build
WORKDIR /retroshare-service-android-build

# ARG declared before FROM get wiped after it, so we need declaring it again
ARG ANDROID_NDK_ARCH=arm
ARG RS_SERVICE_QMAKE_EXTRA_OPTS
RUN $($PREPARE_TOOLCHAIN get_qt_dir | head -n 1)/bin/qmake ../RetroShare \
	-spec android-clang \
	CONFIG+=retroshare_service CONFIG+=rs_jsonapi \
	RS_UPNP_LIB=miniupnpc \
	JSONAPI_GENERATOR_EXE=/jsonapi-generator-build/jsonapi-generator \
	NATIVE_LIBS_TOOLCHAIN_PATH=$NATIVE_LIBS_TOOLCHAIN_PATH \
	CONFIG+=no_retroshare_gui CONFIG+=no_rs_service_webui_terminal_password \
	CONFIG+=no_rs_service_terminal_login \
	CONFIG+=no_rs_sam3 CONFIG+=no_rs_sam3_libsam3 \
	$RS_SERVICE_QMAKE_EXTRA_OPTS
RUN	make -j$(nproc)
RUN make install INSTALL_ROOT=/retroshare-service-android-build/android-build/

ARG ANDROID_PLATFORM_VER=16
RUN $($PREPARE_TOOLCHAIN get_qt_dir | head -n 1)/bin/androiddeployqt \
	--input retroshare-service/src/android-libretroshare-service.so-deployment-settings.json \
	--output android-build --android-platform android-$ANDROID_PLATFORM_VER \
	--jdk $JAVA_HOME --gradle
