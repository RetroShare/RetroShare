#!/bin/sh

[ -n "$CI_MERGE_REQUEST_ID" ] &&
	echo \
		--build-arg REPO_URL="$CI_MERGE_REQUEST_SOURCE_PROJECT_URL" \
		--build-arg REPO_BRANCH="$CI_MERGE_REQUEST_SOURCE_BRANCH_NAME" ||
	echo \
		--build-arg REPO_URL="$CI_REPOSITORY_URL" \
		--build-arg REPO_BRANCH="$CI_COMMIT_BRANCH"
