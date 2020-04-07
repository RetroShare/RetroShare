## Add +--build-arg FRESHCLONE=$(date +%s)+ to docker build commandline to
## force cloning a new

## To prepare an image suitable as base for Gitlab CI use
# docker build -t "${CI_REGISTRY_IMAGE}:base" --build-arg KEEP_SOURCE=true --build-arg REPO_DEPTH="" -f base.Dockerfile .

## Now you need to tag it so you can later push it
# docker tag ${ID_OF_THE_CREATED_IMAGE} registry.gitlab.com/retroshare/${CI_REGISTRY_IMAGE}:base

## To push it to gitlab CI registry you need first to login and the to push
# docker login registry.gitlab.com
# docker push registry.gitlab.com/retroshare/${CI_REGISTRY_IMAGE}:base


## To run the container
# docker run -it -p 127.0.0.1:9092:9092 "${CI_REGISTRY_IMAGE}:base" retroshare-service --jsonApiPort 9092 --jsonApiBindAddress 0.0.0.0

FROM ubuntu

ARG CACHEBUST=0
RUN \
	apt-get update -y && apt-get upgrade -y && \
	apt-get install -y build-essential libssl-dev libbz2-dev libsqlite3-dev \
		libsqlcipher-dev libupnp-dev pkg-config libz-dev \
		qt5-default libxapian-dev qttools5-dev doxygen rapidjson-dev \
		git cmake

ARG FRESHCLONE=0
ARG REPO_URL=https://gitlab.com/RetroShare/RetroShare.git
ARG REPO_BRANCH=master
ARG REPO_DEPTH="--depth 2000"
ARG KEEP_SOURCE=false
RUN apt-get update -y && apt-get upgrade -y
RUN git clone $REPO_DEPTH $REPO_URL -b $REPO_BRANCH && cd RetroShare && \
	git fetch --tags && cd ..
RUN \
	mkdir RetroShare-build && cd RetroShare-build && \
	qmake ../RetroShare \
		CONFIG+=no_retroshare_plugins CONFIG+=ipv6 \
		CONFIG+=retroshare_service CONFIG+=no_retroshare_gui \
		CONFIG+=rs_jsonapi CONFIG+=rs_deep_search && \
	(make -j$(nproc) || make -j$(nproc) || make) && make install && \
	cd .. && rm -rf RetroShare-build && ($KEEP_SOURCE || rm -rf RetroShare)
