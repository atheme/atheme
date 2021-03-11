# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
# Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_FEATURETEST_FHSPATHS], [

    DATADIR='${prefix}/etc'
    DOCDIR='${prefix}/doc'
    LOCALEDIR='${datadir}/locale'
    LOGDIR='${prefix}/var'
    MODDIR='${exec_prefix}'
    RUNDIR='${prefix}/var'
    SHAREDIR='${prefix}'

    AC_ARG_ENABLE([fhs-paths],
        [AS_HELP_STRING([--enable-fhs-paths], [Use more FHS-like pathnames (for packagers)])],
        [], [enable_fhs_paths="no"])

    AS_CASE(["x${enable_fhs_paths}"], [xno], [], [xyes], [
        AC_DEFINE([ATHEME_ENABLE_FHS_PATHS], [1], [Define to 1 if --enable-fhs-paths was given to ./configure])
        DATADIR='${localstatedir}/lib/atheme'
        DOCDIR='${datadir}/doc/atheme'
        LOGDIR='${localstatedir}/log/atheme'
        MODDIR='${libdir}/atheme'
        RUNDIR='${localstatedir}/run/atheme'
        SHAREDIR='${datadir}/atheme'
    ], [
        AC_MSG_ERROR([invalid option for --enable-fhs-paths])
    ])

    AC_SUBST([DATADIR])
    AC_SUBST([DOCDIR])
    AC_SUBST([LOCALEDIR])
    AC_SUBST([LOGDIR])
    AC_SUBST([MODDIR])
    AC_SUBST([RUNDIR])
    AC_SUBST([SHAREDIR])
])
