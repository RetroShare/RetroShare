FROM registry.gitlab.com/retroshare/retroshare:base

RUN apt-get update -y && apt-get upgrade -y

ARG REPO_URL=https://gitlab.com/RetroShare/RetroShare.git
ARG REPO_BRANCH=master
RUN \
	cd RetroShare && git remote add testing $REPO_URL && \
	git fetch --tags testing $REPO_BRANCH && \
	git reset --hard testing/$REPO_BRANCH && \
	git submodule update --init --remote --force \
		libbitdht/ libretroshare/ openpgpsdk/ && \
	git --no-pager log --max-count 1
RUN \
	mkdir RetroShare-build && cd RetroShare-build && \
	cmake -B. -S../RetroShare/retroshare-service \
		-DRS_FORUM_DEEP_INDEX=ON -DRS_JSON_API=ON \
		-DRS_WARN_DEPRECATED=OFF -DRS_WARN_LESS=ON && \
	make -j$(nproc) && make install && \
	cd .. && rm -rf RetroShare-build
