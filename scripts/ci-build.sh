#!/usr/bin/env bash
#
# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2018-2020 Atheme Development Group (https://atheme.github.io/)
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

case "x${CC:-}" in
    xclang*)
        LDFLAGS="-fuse-ld=lld"
        ;;
esac

./configure                                                 \
    --prefix="${ATHEME_PREFIX}"                             \
    --disable-heap-allocator                                \
    --disable-linker-defs                                   \
    --enable-compiler-sanitizers                            \
    --enable-crypto-benchmarking                            \
    --enable-ecdh-x25519-tool                               \
    --enable-ecdsa-nist256p-tools                           \
    --enable-legacy-pwcrypto                                \
    --enable-nls                                            \
    --enable-reproducible-builds                            \
    --enable-warnings                                       \
    --without-libmowgli                                     \
    --with-perl                                             \
    --with-digest-api-frontend=internal                     \
    --with-rng-api-frontend=internal                        \
    ${ATHEME_CONF_ARGS:-}                                   \
    LDFLAGS="${LDFLAGS:-}"

"${MAKE}"
"${MAKE}" install

"${ATHEME_PREFIX}"/bin/atheme-crypto-benchmark -T
"${ATHEME_PREFIX}"/bin/atheme-ecdh-x25519-tool -T
