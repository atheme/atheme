# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2018-2021 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_HANDLE_SUBMODULES], [

    AS_IF([test "${LIBMOWGLI_SOURCE}" = "Internal"], [

        AC_SUBST([SUBMODULE_LIBMOWGLI], [libmowgli-2])

        # Run ./configure in the libmowgli submodule directory but without OpenSSL support
        # Atheme does not use the OpenSSL VIO interface in Mowgli and this avoids an unnecessary -lssl
        # The arguments to remove (last macro argument) must be on one line

        AX_SUBDIRS_CONFIGURE([libmowgli-2], [
            [--without-openssl],
            [CC="\${CC}"],
            [LD="\${LD}"],
            [CFLAGS="\${CFLAGS}"],
            [CPPFLAGS="\${CPPFLAGS}"],
            [LDFLAGS="\${LDFLAGS}"],
            [LIBS="\${LIBS}"]
        ], [[]], [[]], [[--with-openssl], [--with-openssl=auto], [--with-openssl=yes]])
    ])

    AS_IF([test "${CONTRIB_MODULES}" = "Yes"], [

        AC_SUBST([SUBMODULE_CONTRIB], [contrib])

        AX_SUBDIRS_CONFIGURE([modules/contrib], [
            [CC="\${CC}"],
            [LD="\${LD}"],
            [CFLAGS="\${CFLAGS}"],
            [CPPFLAGS="\${CPPFLAGS}"],
            [LDFLAGS="\${LDFLAGS}"],
            [LIBS="\${LIBS}"]
        ], [[]], [[]], [[]])
    ])
])
