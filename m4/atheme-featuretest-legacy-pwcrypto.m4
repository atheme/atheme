# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
# Copyright (C) 2018-2019 Aaron Jones <aaronmdjones@gmail.com>
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_FEATURETEST_LEGACY_PWCRYPTO], [

    LEGACY_PWCRYPTO="No"

    AC_ARG_ENABLE([legacy-pwcrypto],
        [AS_HELP_STRING([--enable-legacy-pwcrypto], [Enable legacy password crypto modules])],
        [], [enable_legacy_pwcrypto="no"])

    case "x${enable_legacy_pwcrypto}" in
        xyes)
            LEGACY_PWCRYPTO="Yes"
            AC_DEFINE([ATHEME_ENABLE_LEGACY_PWCRYPTO], [1], [Define to 1 if --enable-legacy-pwcrypto was given to ./configure])
            ATHEME_COND_LEGACY_PWCRYPTO_ENABLE
            ;;
        xno)
            LEGACY_PWCRYPTO="No"
            ;;
        *)
            AC_MSG_ERROR([invalid option for --enable-legacy-pwcrypto])
            ;;
    esac
])
