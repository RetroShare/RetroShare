#!/bin/sh

# Emit --build-arg flags so gitlabCI.Dockerfile fetches the EXACT ref that
# triggered the pipeline. REPO_REF can be a commit SHA (the CI default), a
# branch name, or a tag. The remote URL is still chosen per-case so
# MR-from-fork fetches from the fork.

if [ -n "$CI_MERGE_REQUEST_ID" ]; then
	REPO_URL="$CI_MERGE_REQUEST_SOURCE_PROJECT_URL"
else
	REPO_URL="$CI_REPOSITORY_URL"
fi

echo \
	--build-arg REPO_URL="$REPO_URL" \
	--build-arg REPO_REF="$CI_COMMIT_SHA"
