# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
# Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_LIBTEST_DL], [

    CFLAGS_SAVED="${CFLAGS}"
    LIBS_SAVED="${LIBS}"

    LIBDL_LIBS=""

    AC_SEARCH_LIBS([dlinfo], [dl], [
        AC_CHECK_HEADERS([dlfcn.h link.h], [], [], [])
        AC_MSG_CHECKING([if dlinfo appears to be usable])
        AC_LINK_IFELSE([
            AC_LANG_PROGRAM([[
                #ifdef HAVE_STDDEF_H
                #  include <stddef.h>
                #endif
                #ifdef HAVE_DLFCN_H
                #  include <dlfcn.h>
                #endif
                #ifdef HAVE_LINK_H
                #  include <link.h>
                #endif
            ]], [[
                struct link_map *map = NULL;
                (void) dlinfo(NULL, RTLD_DI_LINKMAP, &map);
                (void) map->l_addr;
            ]])
        ], [
            AC_MSG_RESULT([yes])
            AC_DEFINE([HAVE_USABLE_DLINFO], [1], [Define to 1 if dlinfo(3) appears to be usable])
            AS_IF([test "x${ac_cv_search_dlinfo}" != "xnone required"], [
                LIBDL_LIBS="${ac_cv_search_dlinfo}"
                AC_SUBST([LIBDL_LIBS])
            ])
        ], [
            AC_MSG_RESULT([no])
        ])
    ], [])

    CFLAGS="${CFLAGS_SAVED}"
    LIBS="${LIBS_SAVED}"

    unset CFLAGS_SAVED
    unset LIBS_SAVED
])
