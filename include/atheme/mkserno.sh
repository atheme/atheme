#!/bin/sh
#
# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
#
# atheme-services: A collection of minimalist IRC services
# mkserno.sh: Stores git commit hash into serno.h
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.

SERNO_FILE="serno.h"

REVH_NEW="$(git log -n1 --abbrev=20 --format='%h')"

if [ "x${REVH_NEW}" = "x" ]
then
	REVH_NEW="<unknown>"
else
	if [ -n "$(git diff --stat)" -o -n "$(git diff --cached --stat)" ]
	then
		REVH_NEW="${REVH_NEW}-dirty"
	fi
fi

REVH_NEW="\"${REVH_NEW}\""

if [ -f "${SERNO_FILE}" ]
then
	REVH_CUR="$(awk '/define SERNO/ { print $3 }' < "${SERNO_FILE}")"

	if [ "x${REVH_NEW}" = "x${REVH_CUR}" ]
	then
		exit 0
	fi
fi

echo "Generate: ${SERNO_FILE}"

cat <<- EOF > "${SERNO_FILE}"

#ifndef ATHEME_INC_SERNO_H
#define ATHEME_INC_SERNO_H 1

#define SERNO ${REVH_NEW}

#endif /* !ATHEME_INC_SERNO_H */

EOF
