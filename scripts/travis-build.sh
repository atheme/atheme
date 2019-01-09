#!/bin/bash
#
# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
#
# Bash script to execute the build and test steps necessary for Travis CI
# <https://travis-ci.org/atheme/atheme/>

PREFIX="/home/travis/atheme-install"

set -e
set -x

./configure                            \
    --prefix="${PREFIX}"               \
    --with-cracklib                    \
    --with-crypt                       \
    --with-ldap                        \
    --with-libidn                      \
    --without-mbedtls                  \
    --with-nettle                      \
    --with-openssl                     \
    --with-pcre                        \
    --with-perl                        \
    --with-qrencode                    \
    --with-sodium                      \
    --enable-debugging                 \
    --enable-legacy-pwcrypto           \
    --enable-nls                       \
    --enable-reproducible-builds       \
    --enable-sodium-malloc             \
    --enable-warnings                  \
    ${OPTIONAL_CONF_ARGS}

make
make install

"${PREFIX}"/bin/atheme-services -dnT
