# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2018-2020 Aaron Jones <aaronmdjones@gmail.com>
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_LIBTEST_IDN], [

    CFLAGS_SAVED="${CFLAGS}"
    LIBS_SAVED="${LIBS}"

    LIBIDN="No"
    LIBIDN_PATH=""

    AC_ARG_WITH([idn],
        [AS_HELP_STRING([--without-idn], [Do not attempt to detect GNU libidn (for modules/saslserv/scram-sha)])],
        [], [with_idn="auto"])

    case "x${with_idn}" in
        xno | xyes | xauto)
            ;;
        x/*)
            LIBIDN_PATH="${with_idn}"
            with_idn="yes"
            ;;
        *)
            AC_MSG_ERROR([invalid option for --with-idn])
            ;;
    esac

    AS_IF([test "${with_idn}" != "no"], [
        AS_IF([test -n "${LIBIDN_PATH}"], [
            # Allow for user to provide custom installation directory
            AS_IF([test -d "${LIBIDN_PATH}/include" -a -d "${LIBIDN_PATH}/lib"], [
                LIBIDN_CFLAGS="-I${LIBIDN_PATH}/include"
                LIBIDN_LIBS="-L${LIBIDN_PATH}/lib -lidn"
            ], [
                AC_MSG_ERROR([${LIBIDN_PATH} is not a suitable directory for GNU libidn])
            ])
        ], [test -n "${PKG_CONFIG}"], [
            # Allow for the user to "override" pkg-config without it being installed
            PKG_CHECK_MODULES([LIBIDN], [libidn], [], [])
        ])
        AS_IF([test -n "${LIBIDN_CFLAGS+set}" -a -n "${LIBIDN_LIBS+set}"], [
            # Only proceed with library tests if custom paths were given or pkg-config succeeded
            LIBIDN="Yes"
        ], [
            LIBIDN="No"
            AS_IF([test "${with_idn}" != "no" && test "${with_idn}" != "auto"], [
                AC_MSG_FAILURE([--with-idn was given but GNU libidn could not be found])
            ])
        ])
    ])

    AS_IF([test "${LIBIDN}" = "Yes"], [
        CFLAGS="${LIBIDN_CFLAGS} ${CFLAGS}"
        LIBS="${LIBIDN_LIBS} ${LIBS}"

        AC_MSG_CHECKING([if GNU libidn appears to be usable])
        AC_LINK_IFELSE([
            AC_LANG_PROGRAM([[
                #ifdef HAVE_STDDEF_H
                #  include <stddef.h>
                #endif
                #include <stringprep.h>
            ]], [[
                (void) stringprep_locale_to_utf8(NULL);
                (void) stringprep(NULL, 0, (Stringprep_profile_flags) 0, stringprep_saslprep);
            ]])
        ], [
            AC_MSG_RESULT([yes])
            FEATURE_SASL_SCRAM="Yes"
            LIBIDN="Yes"
            AC_DEFINE([HAVE_LIBIDN], [1], [Define to 1 if GNU libidn appears to be usable])
        ], [
            AC_MSG_RESULT([no])
            LIBIDN="No"
            AS_IF([test "${with_idn}" != "no" && test "${with_idn}" != "auto"], [
                AC_MSG_ERROR([--with-idn was given but GNU libidn could not be found])
            ])
        ])
    ])

    AS_IF([test "${LIBIDN}" = "No"], [
        LIBIDN_CFLAGS=""
        LIBIDN_LIBS=""
    ])

    AC_SUBST([LIBIDN_CFLAGS])
    AC_SUBST([LIBIDN_LIBS])

    CFLAGS="${CFLAGS_SAVED}"
    LIBS="${LIBS_SAVED}"
])
