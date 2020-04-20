FROM registry.gitlab.com/retroshare/retroshare:base

RUN apt-get update -y && apt-get upgrade -y

ARG REPO_URL=https://gitlab.com/RetroShare/RetroShare.git
ARG REPO_BRANCH=master
RUN \
	cd RetroShare && git remote add testing $REPO_URL && \
	git fetch --tags testing $REPO_BRANCH && \
	git reset --hard testing/$REPO_BRANCH && \
	git --no-pager log --max-count 1
RUN \
	mkdir RetroShare-build && cd RetroShare-build && \
	qmake ../RetroShare CONFIG+=no_retroshare_gui \
		CONFIG+=retroshare_service \
		CONFIG+=rs_jsonapi CONFIG+=rs_deep_search && \
	(make -j$(nproc) || make -j$(nproc) || make) && make install && \
	cd .. && rm -rf RetroShare-build
