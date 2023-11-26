#!/bin/bash

function git_del_tag()
{
	mTag=$1

	for mRemote in $(git remote); do
		echo "Attempting tag $mTag removal from remote $mRemote"
		GIT_TERMINAL_PROMPT=0 git push $mRemote :$mTag || true
	done
	git tag --delete $mTag
}

for mModule in . build_scripts/OBS/ libbitdht/ libretroshare/ openpgpsdk/ retroshare-webui/ ; do
	pushd $mModule
	git_del_tag v0.6.7a
	git tag --list | grep untagged | while read mTag; do git_del_tag $mTag ; done
	popd
done

