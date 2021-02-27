# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
# Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_LIBTEST_SOCKET], [

    LIBS_SAVED="${LIBS}"

    LIBSOCKET_LIBS=""

    AC_CHECK_HEADERS([netinet/in.h sys/socket.h sys/types.h], [], [], [])

    AC_SEARCH_LIBS([socket], [socket], [
        AS_IF([test "x${ac_cv_search_socket}" != "xnone required"], [
            LIBSOCKET_LIBS="${ac_cv_search_socket}"
        ])
    ], [
        AC_MSG_ERROR([socket functions appear to be unusable])
    ])

    LIBS="${LIBSOCKET_LIBS} ${LIBS_SAVED}"

    AC_MSG_CHECKING([if socket functions are usable])
    AC_LINK_IFELSE([
        AC_LANG_PROGRAM([[
            #ifdef HAVE_SYS_TYPES_H
            #  include <sys/types.h>
            #endif
            #ifdef HAVE_SYS_SOCKET_H
            #  include <sys/socket.h>
            #endif
            #ifdef HAVE_NETINET_IN_H
            #  include <netinet/in.h>
            #endif
        ]], [[
            const int fd1 = socket(AF_UNIX, SOCK_DGRAM, 0);
            const int fd2 = socket(AF_UNIX, SOCK_STREAM, 0);
            const int fd3 = socket(AF_INET, SOCK_DGRAM, 0);
            const int fd4 = socket(AF_INET, SOCK_STREAM, 0);
            const int fd5 = socket(AF_INET6, SOCK_DGRAM, 0);
            const int fd6 = socket(AF_INET6, SOCK_STREAM, 0);
        ]])
    ], [
        AC_MSG_RESULT([yes])
    ], [
        AC_MSG_RESULT([no])
        AC_MSG_FAILURE([socket functions appear to be unusable])
    ])

    AC_SUBST([LIBSOCKET_LIBS])

    LIBS="${LIBS_SAVED}"

    unset LIBS_SAVED
])
