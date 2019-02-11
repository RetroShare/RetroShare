#!/bin/bash

# Make sure you have built RetroShare at list once from the same source tree,
# so the support libs get ingluded in the source tarball

## Define default value for variable, take two arguments, $1 variable name,
## $2 default variable value, if the variable is not already define define it
## with default value.
function define_default_value()
{
	VAR_NAME="${1}"
	DEFAULT_VALUE="${2}"

	[ -z "${!VAR_NAME}" ] && export ${VAR_NAME}="${DEFAULT_VALUE}"
}

define_default_value GIT_DIR "${HOME}/Development/rs-develop/.git"
define_default_value WORK_DIR "$(mktemp --directory)/"

ORIG_DIR="$(pwd)"

[ "$(ls "${GIT_DIR}/../supportlibs/restbed/" | wc -l)" -lt "5" ] &&
{
	cat << EOF
WARNING: supportlibs/restbed/ seems have not been checked out!
The produced tarball may not be suitable to build RetroShare JSON API
EOF
}

cd "${WORK_DIR}"
rsync -a --delete \
	--exclude='.git' \
	--filter=':- build_scripts/OBS/.gitignore' \
	"${GIT_DIR}/../" RetroShare/
git describe > RetroShare/Source_Version
tar -zcf RetroShare.tar.gz RetroShare/

cat RetroShare/Source_Version
md5sum RetroShare.tar.gz
wc -c RetroShare.tar.gz
mv RetroShare.tar.gz "${ORIG_DIR}/RetroShare.tar.gz"
rm -rf "${WORK_DIR}" 

