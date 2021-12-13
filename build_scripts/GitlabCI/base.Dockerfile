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

FROM ubuntu

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y && apt-get upgrade -y -qq && \
	apt-get install -y -qq build-essential cimg-dev libssl-dev libbz2-dev \
		libsqlite3-dev \
		libsqlcipher-dev libupnp-dev pkg-config libz-dev \
		qt5-default libxapian-dev qttools5-dev doxygen rapidjson-dev \
		git cmake curl

RUN git clone --depth 1 https://github.com/aetilius/pHash.git && \
	rm -rf pHash-build && mkdir pHash-build && cd pHash-build && \
	cmake -B. -H../pHash -DCMAKE_INSTALL_PREFIX=/usr && \
	make -j$(nproc) && make install && cd .. && \
	rm -rf pHash-build pHash

ARG FRESHCLONE=0
ARG REPO_URL=https://gitlab.com/RetroShare/RetroShare.git
ARG REPO_BRANCH=master
ARG REPO_DEPTH="--depth 2000"
RUN git clone $REPO_DEPTH $REPO_URL -b $REPO_BRANCH && cd RetroShare && \
	git fetch --tags && cd ..
RUN mkdir RetroShare-build && cd RetroShare-build && \
	qmake ../RetroShare \
		CONFIG+=no_retroshare_plugins \
		CONFIG+=retroshare_service CONFIG+=no_retroshare_gui \
		CONFIG+=rs_jsonapi CONFIG+=rs_deep_search && \
	(make -j$(nproc) || make -j$(nproc) || make) && make install && \
	cd .. && rm -rf RetroShare-build
