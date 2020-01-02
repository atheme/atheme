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

set -euxo pipefail

if [[ -z "${HOME:-}" ]]
then
	echo "HOME is unset" >&2
	exit 1
fi

[[ -z "${MAKE:-}" ]] && MAKE="make"

ATHEME_PREFIX="${HOME}/atheme-install"

./configure                             \
    --prefix="${ATHEME_PREFIX}"         \
    --with-perl                         \
    --enable-debugging                  \
    --enable-legacy-pwcrypto            \
    --enable-nls                        \
    --enable-reproducible-builds        \
    --with-digest-api-frontend=internal \
    --with-rng-api-frontend=internal    \
    ${ATHEME_CONF_ARGS:-}

"${MAKE}"
"${MAKE}" install

"${ATHEME_PREFIX}"/bin/atheme-services -dnT
"${ATHEME_PREFIX}"/bin/atheme-crypto-benchmark -o
