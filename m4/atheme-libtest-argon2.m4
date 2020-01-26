# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2018-2020 Aaron Jones <aaronmdjones@gmail.com>
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_LIBTEST_ARGON2], [

    CFLAGS_SAVED="${CFLAGS}"
    LIBS_SAVED="${LIBS}"

    LIBARGON2="No"
    LIBARGON2_PATH=""

    AC_ARG_WITH([argon2],
        [AS_HELP_STRING([--without-argon2], [Do not attempt to detect libargon2 (for modules/crypto/argon2)])],
        [], [with_argon2="auto"])

    case "x${with_argon2}" in
        xno | xyes | xauto)
            ;;
        x/*)
            LIBARGON2_PATH="${with_argon2}"
            with_argon2="yes"
            ;;
        *)
            AC_MSG_ERROR([invalid option for --with-argon2])
            ;;
    esac

    AS_IF([test "${with_argon2}" != "no"], [
        AS_IF([test -n "${LIBARGON2_PATH}"], [
            # Allow for user to provide custom installation directory
            AS_IF([test -d "${LIBARGON2_PATH}/include" -a -d "${LIBARGON2_PATH}/lib"], [
                LIBARGON2_CFLAGS="-I${LIBARGON2_PATH}/include"
                LIBARGON2_LIBS="-L${LIBARGON2_PATH}/lib -largon2"
            ], [
                AC_MSG_ERROR([${LIBARGON2_PATH} is not a suitable directory for libargon2])
            ])
        ], [test -n "${PKG_CONFIG}"], [
            # Allow for the user to "override" pkg-config without it being installed
            PKG_CHECK_MODULES([LIBARGON2], [libargon2], [], [])
        ])
        AS_IF([test -n "${LIBARGON2_CFLAGS+set}" -a -n "${LIBARGON2_LIBS+set}"], [
            # Only proceed with library tests if custom paths were given or pkg-config succeeded
            LIBARGON2="Yes"
        ], [
            LIBARGON2="No"
            AS_IF([test "${with_argon2}" != "no" && test "${with_argon2}" != "auto"], [
                AC_MSG_FAILURE([--with-argon2 was given but libargon2 could not be found])
            ])
        ])
    ])

    AS_IF([test "${LIBARGON2}" = "Yes"], [
        CFLAGS="${LIBARGON2_CFLAGS} ${CFLAGS}"
        LIBS="${LIBARGON2_LIBS} ${LIBS}"

        AC_MSG_CHECKING([if libargon2 appears to be usable])
        AC_LINK_IFELSE([
            AC_LANG_PROGRAM([[
                #ifdef HAVE_STDDEF_H
                #  include <stddef.h>
                #endif
                #ifdef HAVE_SIGNAL_H
                #  include <signal.h>
                #endif
                #include <argon2.h>
            ]], [[
                // POSIX.1-2001 Signal-blocking functions are necessary for thread-safety
                sigset_t oldset;
                sigset_t newset;
                (void) sigfillset(&newset);
                (void) sigprocmask(SIG_BLOCK, &newset, &oldset);

                argon2_context ctx = {
                    .out = NULL,
                    .outlen = 0,
                    .pwd = NULL,
                    .pwdlen = 0,
                    .salt = NULL,
                    .saltlen = 0,
                    .t_cost = ARGON2_MAX_TIME,
                    .m_cost = ARGON2_MAX_MEMORY,
                    .lanes = 1,
                    .threads = 1,
                    .version = ARGON2_VERSION_NUMBER,
                };
                (void) argon2_ctx(&ctx, Argon2_d);
                (void) argon2_ctx(&ctx, Argon2_i);
                (void) argon2_ctx(&ctx, Argon2_id);
                (void) argon2_error_message(0);
                (void) argon2_type2string(0, 0);
            ]])
        ], [
            AC_MSG_RESULT([yes])
            LIBARGON2="Yes"
            AC_DEFINE([HAVE_LIBARGON2], [1], [Define to 1 if libargon2 appears to be usable])
        ], [
            AC_MSG_RESULT([no])
            LIBARGON2="No"
            AS_IF([test "${with_argon2}" != "no" && test "${with_argon2}" != "auto"], [
                AC_MSG_FAILURE([--with-argon2 was given but libargon2 appears to be unusable])
            ])
        ])
    ])

    AS_IF([test "${LIBARGON2}" = "No"], [
        LIBARGON2_CFLAGS=""
        LIBARGON2_LIBS=""
    ])

    AC_SUBST([LIBARGON2_CFLAGS])
    AC_SUBST([LIBARGON2_LIBS])

    CFLAGS="${CFLAGS_SAVED}"
    LIBS="${LIBS_SAVED}"
])
