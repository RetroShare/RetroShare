## Add +--build-arg FRESHCLONE=$(date +%s)+ to docker build commandline to
## force cloning a new

## To prepare an image suitable as base for Gitlab CI use
# export CI_REGISTRY_IMAGE="registry.gitlab.com/retroshare/retroshare:base"
# docker build -t "${CI_REGISTRY_IMAGE}" -f base.Dockerfile .

## To push it to gitlab CI registry you need first to login and the to push
# docker login registry.gitlab.com
# docker push "${CI_REGISTRY_IMAGE}"

## To run the container
# docker run -it -p 127.0.0.1:9092:9092 "${CI_REGISTRY_IMAGE}" retroshare-service --jsonApiPort 9092 --jsonApiBindAddress 0.0.0.0

FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV APT_UNAT="--assume-yes --quiet"

RUN apt-get update $APT_UNAT && \
	apt-get upgrade --show-upgraded $APT_UNAT && \
	apt-get clean $APT_UNAT && \
	apt-get install --no-install-recommends $APT_UNAT \
		bash build-essential cimg-dev libssl-dev libbz2-dev \
		libminiupnpc-dev \
		libsqlite3-dev libsqlcipher-dev \
		pkg-config libz-dev \
		libxapian-dev doxygen rapidjson-dev \
		git cmake curl python3

## Avoid git cloning spuriously failing with
#  server certificate verification failed. CAfile: none CRLfile: none
RUN apt-get install --no-install-recommends $APT_UNAT --reinstall \
	ca-certificates

RUN git clone --depth 1 https://github.com/aetilius/pHash.git && \
	rm -rf pHash-build && mkdir pHash-build && cd pHash-build && \
	cmake -B. -H../pHash -DCMAKE_INSTALL_PREFIX=/usr && \
	make -j$(nproc) && make install && cd .. && \
	rm -rf pHash-build pHash

ARG FRESHCLONE=0
ARG REPO_URL=https://github.com/RetroShare/RetroShare.git
ARG REPO_BRANCH=master
ARG REPO_DEPTH="--depth 2000"
RUN git clone $REPO_DEPTH $REPO_URL -b $REPO_BRANCH && \
	cd RetroShare && \
	git fetch --tags && \
	git submodule update --init \
		libbitdht/ libretroshare/ openpgpsdk/ retroshare-webui/ \
		supportlibs/restbed/ && \
	cd supportlibs/restbed/ && \
	git submodule update --init \
		dependency/asio/ dependency/kashmir/ && \
	cd ../../../

RUN \
	mkdir RetroShare-build && cd RetroShare-build && \
	cmake -B. -S../RetroShare/retroshare-service \
		-DRS_FORUM_DEEP_INDEX=ON -DRS_JSON_API=ON -DRS_WEBUI=ON && \
	make -j$(nproc) && make install && \
	cd .. && rm -rf RetroShare-build
