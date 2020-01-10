# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
# Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

ATHEME_LIBTEST_CONTRIB_RES_QUERY_RESULT=""

AC_DEFUN([ATHEME_LIBTEST_CONTRIB_GETHOSTBYNAME], [

    AC_SEARCH_LIBS([gethostbyname], [nsl], [
        AS_IF([test "x${ac_cv_search_gethostbyname}" != "xnone required"], [
            CONTRIB_LIBS="${ac_cv_search_gethostbyname} ${CONTRIB_LIBS}"
        ])
    ], [
        AC_MSG_ERROR([--enable-contrib needs a functioning gethostbyname(3)])
    ], [])
])

AC_DEFUN([ATHEME_LIBTEST_CONTRIB_RES_QUERY_DRIVER], [

    AC_LINK_IFELSE([
        AC_LANG_PROGRAM([[
            #ifdef HAVE_STDDEF_H
            #  include <stddef.h>
            #endif
            #ifdef HAVE_SYS_TYPES_H
            #  include <sys/types.h>
            #endif
            #ifdef HAVE_NETINET_IN_H
            #  include <netinet/in.h>
            #endif
            #ifdef HAVE_ARPA_NAMESER_H
            #  include <arpa/nameser.h>
            #endif
            #ifdef HAVE_NETDB_H
            #  include <netdb.h>
            #endif
            #ifdef HAVE_RESOLV_H
            #  include <resolv.h>
            #endif
        ]], [[
            const int ret = res_query(NULL, ns_c_any, ns_t_mx, NULL, 0);
        ]])
    ], [
        ATHEME_LIBTEST_CONTRIB_RES_QUERY_RESULT="yes"
    ], [
        ATHEME_LIBTEST_CONTRIB_RES_QUERY_RESULT="no"
    ])
])

AC_DEFUN([ATHEME_LIBTEST_CONTRIB_RES_QUERY], [

    AC_HEADER_RESOLV

    AC_MSG_CHECKING([whether compiling and linking a program using res_query(3) works])

    ATHEME_LIBTEST_CONTRIB_RES_QUERY_DRIVER

    AS_IF([test "${ATHEME_LIBTEST_CONTRIB_RES_QUERY_RESULT}" = "yes"], [
        AC_MSG_RESULT([yes])
    ], [
        LIBS="-lresolv ${LIBS}"

        ATHEME_LIBTEST_CONTRIB_RES_QUERY_DRIVER

        AS_IF([test "${ATHEME_LIBTEST_CONTRIB_RES_QUERY_RESULT}" = "yes"], [
            AC_MSG_RESULT([yes])
            CONTRIB_LIBS="-lresolv ${CONTRIB_LIBS}"
        ], [
            AC_MSG_RESULT([no])
            AC_MSG_ERROR([--enable-contrib needs a functioning res_query(3)])
        ])
    ])
])

AC_DEFUN([ATHEME_LIBTEST_CONTRIB], [

    CONTRIB_LIBS="${LIBMATH_LIBS} ${LIBSOCKET_LIBS}"
    LIBS_SAVED="${LIBS}"
    ATHEME_LIBTEST_CONTRIB_GETHOSTBYNAME
    ATHEME_LIBTEST_CONTRIB_RES_QUERY
    LIBS="${LIBS_SAVED}"
    AC_SUBST([CONTRIB_LIBS])
])

AC_DEFUN([ATHEME_FEATURETEST_CONTRIB], [

    CONTRIB_MODULES="No"

    AC_ARG_ENABLE([contrib],
        [AS_HELP_STRING([--enable-contrib], [Enable contrib modules])],
        [], [enable_contrib="no"])

    case "x${enable_contrib}" in
        xyes)
            ATHEME_LIBTEST_CONTRIB
            CONTRIB_MODULES="Yes"
            AC_DEFINE([ATHEME_ENABLE_CONTRIB], [1], [Define to 1 if --enable-contrib was given to ./configure])
            ATHEME_COND_CONTRIB_SUBMODULE_ENABLE
            ;;
        xno)
            CONTRIB_MODULES="No"
            ;;
        *)
            AC_MSG_ERROR([invalid option for --enable-contrib])
            ;;
    esac
])
