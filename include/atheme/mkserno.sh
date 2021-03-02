#!/bin/sh
#
# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2018-2021 Atheme Development Group (https://atheme.github.io/)
#
# atheme-services: A collection of minimalist IRC services
# mkserno.sh: Stores git revision information into serno.h
#   (for use when building with a git(1) source tree)
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.

if [ "x${SKIP_SERNO_H_GENERATION:-}" = "xyes" ]
then
	exit 0
fi

SERNO_FILE="serno.h"

REVH_NEW="$(git rev-parse --short=20 HEAD)"

if [ "x${REVH_NEW}" = "x" ]
then
	echo " " >&2
	echo "Could not determine git revision; is git(1) installed?" >&2
	echo " " >&2
	echo "If you are not expecting this message, perhaps you" >&2
	echo "downloaded the wrong sourcecode tarball. Please see" >&2
	echo "the GIT-Access.txt file for instructions." >&2
	echo " " >&2
	exit 1
fi

if [ -n "$(git diff --stat)" -o -n "$(git diff --cached --stat)" ]
then
	REVH_NEW="${REVH_NEW}-dirty"
fi

REVH_NEW="\"${REVH_NEW}\""

if [ -f "${SERNO_FILE}" ]
then
	REVH_CUR="$(awk '/define SERNO/ { print $3 }' < "${SERNO_FILE}")"

	if [ "x${REVH_NEW}" = "x${REVH_CUR}" ]
	then
		# If we would write the same information, avoid touching the
		# file altogether, and thus avoid causing a rebuild of the
		# entire source repository next time make is run (due to an
		# updated mtime in combination with dependency tracking).
		exit 0
	fi
fi

echo "Generate: ${SERNO_FILE}"

cat <<-EOF > "${SERNO_FILE}"

#ifndef ATHEME_INC_SERNO_H
#define ATHEME_INC_SERNO_H 1

#define SERNO ${REVH_NEW}

#endif /* !ATHEME_INC_SERNO_H */

EOF
