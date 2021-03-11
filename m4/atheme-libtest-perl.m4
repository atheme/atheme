# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
# Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_LIBTEST_PERL], [

    LIBPERL="No"

    LIBPERL_CFLAGS=""
    LIBPERL_LIBS=""

    AC_ARG_WITH([perl],
            [AS_HELP_STRING([--with-perl], [Enable Perl (for modules/scripting/perl)])],
            [], [with_perl="no"])

    AS_CASE(["x${with_perl}"], [xno], [], [xyes], [], [xauto], [], [
        AC_MSG_ERROR([invalid option for --with-perl])
    ])

    AS_IF([test "${with_perl}" != "no"], [

        AC_PATH_PROG([perlpath], [perl])

        AS_IF([test -n "${perlpath}"], [
            LIBPERL="Yes"
            LIBPERL_CFLAGS="$(perl -MExtUtils::Embed -e ccopts)"
            LIBPERL_LIBS="$(perl -MExtUtils::Embed -e ldopts)"

            # if Perl is built with threading support, we need to link atheme against libpthread
            AS_CASE(["x${LIBPERL_LIBS}"], [*-lpthread*], [LIBS="-lpthread ${LIBS}"])

            AS_IF([test "${with_perl}${LIBPERL_CFLAGS}" = "yes"], [
                AC_MSG_ERROR([--with-perl was given but Perl could not be found])
            ])

            ATHEME_COND_PERL_ENABLE
        ], [
            LIBPERL="No"
            AS_IF([test "${with_perl}" = "yes"], [
                AC_MSG_ERROR([--with-perl was given but Perl could not be found])
            ])
        ])
    ], [
        LIBPERL="No"
    ])

    AS_IF([test "${LIBPERL}" = "No"], [
        LIBPERL_CFLAGS=""
        LIBPERL_LIBS=""
    ])

    AC_SUBST([LIBPERL_CFLAGS])
    AC_SUBST([LIBPERL_LIBS])
])
