# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2015 Atheme Project (http://atheme.org/)
# Copyright (C) 2015-2020 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_REQUIRED_FUNC_MISSING], [

    AC_MSG_ERROR([required function not available])
])

AC_DEFUN([ATHEME_CHECK_BUILD_REQUIREMENTS], [

    AC_DEFINE([__STDC_WANT_LIB_EXT1__], [1], [We want ISO 9899:2011 Annex K memory functions if available])
    AC_USE_SYSTEM_EXTENSIONS

    AC_CHECK_HEADERS([ctype.h], [], [], [])
    AC_CHECK_HEADERS([dirent.h], [], [], [])
    AC_CHECK_HEADERS([errno.h], [], [], [])
    AC_CHECK_HEADERS([inttypes.h], [], [], [])
    AC_CHECK_HEADERS([libintl.h], [], [], [])
    AC_CHECK_HEADERS([limits.h], [], [], [])
    AC_CHECK_HEADERS([locale.h], [], [], [])
    AC_CHECK_HEADERS([netdb.h], [], [], [])
    AC_CHECK_HEADERS([netinet/in.h], [], [], [])
    AC_CHECK_HEADERS([regex.h], [], [], [])
    AC_CHECK_HEADERS([signal.h], [], [], [])
    AC_CHECK_HEADERS([stdarg.h], [], [], [])
    AC_CHECK_HEADERS([stdbool.h], [], [], [])
    AC_CHECK_HEADERS([stddef.h], [], [], [])
    AC_CHECK_HEADERS([stdint.h], [], [], [])
    AC_CHECK_HEADERS([stdio.h], [], [], [])
    AC_CHECK_HEADERS([stdlib.h], [], [], [])
    AC_CHECK_HEADERS([string.h], [], [], [])
    AC_CHECK_HEADERS([strings.h], [], [], [])
    AC_CHECK_HEADERS([sys/file.h], [], [], [])
    AC_CHECK_HEADERS([sys/resource.h], [], [], [])
    AC_CHECK_HEADERS([sys/stat.h], [], [], [])
    AC_CHECK_HEADERS([sys/time.h], [], [], [])
    AC_CHECK_HEADERS([sys/types.h], [], [], [])
    AC_CHECK_HEADERS([sys/wait.h], [], [], [])
    AC_CHECK_HEADERS([time.h], [], [], [])
    AC_CHECK_HEADERS([unistd.h], [], [], [])

    AC_CHECK_FUNCS([consttime_memequal], [], [])
    AC_CHECK_FUNCS([dup2], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([execve], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([explicit_bzero], [], [])
    AC_CHECK_FUNCS([explicit_memset], [], [])
    AC_CHECK_FUNCS([fileno], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([flock], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([fork], [], [])
    AC_CHECK_FUNCS([fsync], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([gethostbyname], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([getpid], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([getrlimit], [], [])
    AC_CHECK_FUNCS([gettimeofday], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([localeconv], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([memchr], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([memmove], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([memset], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([memset_s], [], [])
    AC_CHECK_FUNCS([regcomp], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([regerror], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([regexec], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([regfree], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([setenv], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([setlocale], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([snprintf], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([strcasecmp], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([strcasestr], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([strchr], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([strerror], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([strlen], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([strncasecmp], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([strnlen], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([strpbrk], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([strrchr], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([strstr], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([strtod], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([strtok], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([strtok_r], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([strtol], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([strtold], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([strtoul], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([strtoull], [], [ATHEME_REQUIRED_FUNC_MISSING])
    AC_CHECK_FUNCS([timingsafe_bcmp], [], [])
    AC_CHECK_FUNCS([timingsafe_memcmp], [], [])
    AC_CHECK_FUNCS([vsnprintf], [], [ATHEME_REQUIRED_FUNC_MISSING])

    AC_C_BIGENDIAN
    AC_C_CONST
    AC_C_INLINE
    AC_C_RESTRICT
    AC_TYPE_INT8_T
    AC_TYPE_INT16_T
    AC_TYPE_INT32_T
    AC_TYPE_INT64_T
    AC_TYPE_LONG_DOUBLE
    AC_TYPE_LONG_LONG_INT
    AC_TYPE_OFF_T
    AC_TYPE_PID_T
    AC_CHECK_TYPES([ptrdiff_t])
    AC_TYPE_SIZE_T
    AC_TYPE_SSIZE_T
    AC_TYPE_UID_T
    AC_TYPE_UINT8_T
    AC_TYPE_UINT16_T
    AC_TYPE_UINT32_T
    AC_TYPE_UINT64_T
    AC_TYPE_UINTPTR_T

    # Initialise our build system
    BUILDSYS_INIT
    BUILDSYS_SHARED_LIB
    BUILDSYS_PROG_IMPLIB
    LIBS="${DYNAMIC_LD_LIBS} ${LIBS}"

    # If we're building on Windows we need socket and regex libraries from mingw
    case "${host}" in
        *-*-mingw32)
            CPPFLAGS="-I/mingw/include ${CPPFLAGS}"
            LIBS="-lwsock32 -lws2_32 -lregex ${LIBS}"
            ;;
    esac
])
