# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
# Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_LIBTEST_LDAP], [

    CFLAGS_SAVED="${CFLAGS}"
    LIBS_SAVED="${LIBS}"

    LIBLDAP="No"
    LIBLDAP_CFLAGS=""
    LIBLDAP_LIBS=""

    AC_ARG_WITH([ldap],
        [AS_HELP_STRING([--without-ldap], [Do not attempt to detect libldap (for modules/auth/ldap)])],
        [], [with_ldap="auto"])

    AS_CASE(["x${with_ldap}"], [xno], [], [xyes], [], [xauto], [], [
        AC_MSG_ERROR([invalid option for --with-ldap])
    ])

    AS_IF([test "${with_ldap}" != "no"], [
        # If this library ever starts shipping a pkg-config file, change to PKG_CHECK_MODULES ?
        AC_SEARCH_LIBS([ldap_initialize], [ldap], [
            AC_MSG_CHECKING([if libldap appears to be usable])
            AC_COMPILE_IFELSE([
                AC_LANG_PROGRAM([[
                    #ifdef HAVE_STDDEF_H
                    #  include <stddef.h>
                    #endif
                    #include <ldap.h>
                ]], [[
                    (void) ldap_set_option(NULL, 0, NULL);
                    (void) ldap_initialize(NULL, NULL);
                    (void) ldap_get_dn(NULL, NULL);
                    (void) ldap_err2string(0);
                ]])
            ], [
                AC_MSG_RESULT([yes])
                LIBLDAP="Yes"
                AC_DEFINE([HAVE_LIBLDAP], [1], [Define to 1 if libldap appears to be usable])
                AS_IF([test "x${ac_cv_search_ldap_initialize}" != "xnone required"], [
                    LIBLDAP_LIBS="${ac_cv_search_ldap_initialize}"
                ])
            ], [
                AC_MSG_RESULT([no])
                LIBLDAP="No"
                AS_IF([test "${with_ldap}" = "yes"], [
                    AC_MSG_FAILURE([--with-ldap was given but libldap appears to be unusable])
                ])
            ])
        ], [
            LIBLDAP="No"
            AS_IF([test "${with_ldap}" = "yes"], [
                AC_MSG_ERROR([--with-ldap was given but libldap could not be found])
            ])
        ])
    ], [
        LIBLDAP="No"
    ])

    AS_IF([test "${LDAP}" = "No"], [
        LIBLDAP_CFLAGS=""
        LIBLDAP_LIBS=""
    ])

    AC_SUBST([LIBLDAP_CFLAGS])
    AC_SUBST([LIBLDAP_LIBS])

    CFLAGS="${CFLAGS_SAVED}"
    LIBS="${LIBS_SAVED}"

    unset CFLAGS_SAVED
    unset LIBS_SAVED
])
