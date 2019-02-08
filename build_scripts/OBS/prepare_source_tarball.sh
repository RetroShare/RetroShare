#!/bin/sh

#!/bin/bash

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


rsync -aPh --delete --exclude='.git' "${GIT_DIR}/../" RetroShare/
git describe > RetroShare/Source_Version
tar -zcvf RetroShare.tar.gz RetroShare/
cat RetroShare/Source_Version
md5sum RetroShare.tar.gz
wc -c RetroShare.tar.gz
