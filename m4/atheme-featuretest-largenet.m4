# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
# Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_FEATURETEST_LARGENET], [

    LARGE_NET="No"

    AC_ARG_ENABLE([large-net],
        [AS_HELP_STRING([--enable-large-net], [Enable large network support])],
        [], [enable_large_net="no"])

    case "x${enable_large_net}" in
        xyes)
            LARGE_NET="Yes"
            AC_DEFINE([ATHEME_ENABLE_LARGE_NET], [1], [Define to 1 if --enable-large-net was given to ./configure])
            ;;
        xno)
            LARGE_NET="No"
            ;;
        *)
            AC_MSG_ERROR([invalid option for --enable-large-net])
            ;;
    esac
])
