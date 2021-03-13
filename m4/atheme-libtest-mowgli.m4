# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
# Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_LIBTEST_MOWGLI_DRIVER], [

    CFLAGS_SAVED="${CFLAGS}"
    LIBS_SAVED="${LIBS}"

    CFLAGS="${LIBMOWGLI_CFLAGS} ${CFLAGS}"
    LIBS="${LIBMOWGLI_LIBS} ${LIBS}"

    AC_MSG_CHECKING([if libmowgli appears to be usable])
    AC_LINK_IFELSE([
        AC_LANG_PROGRAM([[
            #ifdef HAVE_STDDEF_H
            #  include <stddef.h>
            #endif
            #include <mowgli.h>
        ]], [[
            (void) mowgli_alloc(0);
            (void) mowgli_allocation_policy_create(NULL, NULL, NULL);
            (void) mowgli_allocator_set_policy(NULL);
            (void) mowgli_config_file_free(NULL);
            (void) mowgli_config_file_load(NULL);
            (void) mowgli_dns_create(NULL, MOWGLI_DNS_TYPE_ASYNC);
            (void) mowgli_dns_delete_query(NULL, NULL);
            (void) mowgli_dns_destroy(NULL);
            (void) mowgli_dns_gethost_byname(NULL, NULL, NULL, 0);
            (void) mowgli_eventloop_create();
            (void) mowgli_eventloop_destroy(NULL);
            (void) mowgli_eventloop_get_time(NULL);
            (void) mowgli_eventloop_ignore_errno(0);
            (void) mowgli_eventloop_run_once(NULL);
            (void) mowgli_eventloop_synchronize(NULL);
            (void) mowgli_free(NULL);
            (void) mowgli_getopt_long(0, NULL, NULL, NULL, NULL);
            (void) mowgli_global_storage_free(NULL);
            (void) mowgli_global_storage_get(NULL);
            (void) mowgli_global_storage_put(NULL, NULL);
            (void) mowgli_heap_alloc(NULL);
            (void) mowgli_heap_create(0, 0, BH_NOW);
            (void) mowgli_heap_destroy(NULL);
            (void) mowgli_heap_free(NULL, NULL);
            (void) mowgli_json_create_string(NULL);
            (void) mowgli_json_parse_string(NULL);
            (void) mowgli_json_serialize_to_string(NULL, NULL, 0);
            (void) mowgli_list_create();
            (void) mowgli_list_free(NULL);
            (void) mowgli_list_reverse(NULL);
            (void) mowgli_list_sort(NULL, NULL, NULL);
            (void) mowgli_module_close(NULL);
            (void) mowgli_module_open(NULL);
            (void) mowgli_module_symbol(NULL, NULL);
            (void) mowgli_node_add(NULL, NULL, NULL);
            (void) mowgli_node_add_before(NULL, NULL, NULL, NULL);
            (void) mowgli_node_add_head(NULL, NULL, NULL);
            (void) mowgli_node_create();
            (void) mowgli_node_delete(NULL, NULL);
            (void) mowgli_node_find(NULL, NULL);
            (void) mowgli_node_free(NULL);
            (void) mowgli_node_nth_data(NULL, 0);
            (void) mowgli_patricia_add(NULL, NULL, NULL);
            (void) mowgli_patricia_create(NULL);
            (void) mowgli_patricia_delete(NULL, NULL);
            (void) mowgli_patricia_destroy(NULL, NULL, NULL);
            (void) mowgli_patricia_foreach(NULL, NULL, NULL);
            (void) mowgli_patricia_foreach_cur(NULL, NULL);
            (void) mowgli_patricia_foreach_next(NULL, NULL);
            (void) mowgli_patricia_foreach_start(NULL, NULL);
            (void) mowgli_patricia_retrieve(NULL, NULL);
            (void) mowgli_patricia_size(NULL);
            (void) mowgli_patricia_stats(NULL, NULL, NULL);
            (void) mowgli_pollable_create(NULL, 0, NULL);
            (void) mowgli_pollable_destroy(NULL, NULL);
            (void) mowgli_pollable_setselect(NULL, NULL, MOWGLI_EVENTLOOP_IO_READ, NULL);
            (void) mowgli_pollable_setselect(NULL, NULL, MOWGLI_EVENTLOOP_IO_WRITE, NULL);
            (void) mowgli_signal_install_handler(0, NULL);
            (void) mowgli_strlcat(NULL, NULL, 0);
            (void) mowgli_strlcpy(NULL, NULL, 0);
            (void) mowgli_thread_set_policy(MOWGLI_THREAD_POLICY_DISABLED);
            (void) mowgli_timer_add(NULL, NULL, NULL, NULL, 0);
            (void) mowgli_timer_add_once(NULL, NULL, NULL, NULL, 0);
            (void) mowgli_timer_destroy(NULL, NULL);
        ]])
    ], [
        AC_MSG_RESULT([yes])
    ], [
        AC_MSG_RESULT([no])
        AS_IF([test "${with_libmowgli}" = "yes"], [
            AC_MSG_FAILURE([--with-libmowgli=yes was given, but the libmowgli detected by pkg-config is not suitable])
        ], [
            AC_MSG_FAILURE([--with-libmowgli=path was given, but the path does not contain a usable libmowgli])
        ])
    ])

    CFLAGS="${CFLAGS_SAVED}"
    LIBS="${LIBS_SAVED}"

    unset CFLAGS_SAVED
    unset LIBS_SAVED
])

AC_DEFUN([ATHEME_LIBTEST_MOWGLI], [

    LIBMOWGLI_SOURCE=""
    LIBMOWGLI_CFLAGS=""
    LIBMOWGLI_LIBS=""

    AC_ARG_WITH([libmowgli],
        [AS_HELP_STRING([--with-libmowgli@<:@=prefix@:>@], [Specify location of system libmowgli install, "yes" to ask pkg-config, or "no" to force use of internal libmowgli submodule (default)])],
        [], [with_libmowgli="no"])

    AS_CASE(["x${with_libmowgli}"], [xno], [
        LIBMOWGLI_SOURCE="Internal"
        LIBMOWGLI_CFLAGS="-I$(pwd)/libmowgli-2/src/libmowgli"
        LIBMOWGLI_LIBS="-L$(pwd)/libmowgli-2/src/libmowgli -lmowgli-2"
    ], [xyes], [
        AS_IF([test -z "${PKG_CONFIG}"], [
            AC_MSG_ERROR([--with-libmowgli=yes was given, but pkg-config is required and is not available])
        ])
        PKG_CHECK_MODULES([LIBMOWGLI], [libmowgli-2 >= 2.0.0], [], [
            AC_MSG_ERROR([--with-libmowgli=yes was given, but pkg-config could not locate libmowgli])
        ])
        LIBMOWGLI_SOURCE="System (pkg-config)"
        ATHEME_LIBTEST_MOWGLI_DRIVER
    ], [x/*], [
        AS_IF([test -d "${with_libmowgli}/include/libmowgli-2" -a -d "${with_libmowgli}/lib"], [], [
            AC_MSG_ERROR([${with_libmowgli} is not a suitable directory for libmowgli])
        ])
        LIBMOWGLI_SOURCE="System (${with_libmowgli})"
        LIBMOWGLI_CFLAGS="-I${with_libmowgli}/include/libmowgli-2"
        LIBMOWGLI_LIBS="-L${with_libmowgli}/lib -lmowgli-2"
        ATHEME_LIBTEST_MOWGLI_DRIVER
    ], [
        AC_MSG_ERROR([invalid option for --with-libmowgli])
    ])

    AC_SUBST([LIBMOWGLI_CFLAGS])
    AC_SUBST([LIBMOWGLI_LIBS])
])
