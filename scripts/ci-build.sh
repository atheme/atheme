#!/usr/bin/env bash
#
# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
#
# Bash script to execute the build and test steps necessary for
# builds on Continuous Integration services (e.g. Travis CI).
#
# <https://travis-ci.org/atheme/atheme/>

if [[ "x${HOME}" == "x" ]]
then
	echo "HOME is unset" >&2
	exit 1
fi

[[ "x${MAKE}" == "x" ]] && MAKE="make"

ATHEME_PREFIX="${HOME}/atheme-install"

set -e
set -x

./configure                            \
    --prefix="${ATHEME_PREFIX}"        \
    --with-perl                        \
    --enable-debugging                 \
    --enable-legacy-pwcrypto           \
    --enable-nls                       \
    --enable-reproducible-builds       \
    ${ATHEME_CONF_ARGS}

"${MAKE}"
"${MAKE}" install

"${ATHEME_PREFIX}"/bin/atheme-services -dnT
