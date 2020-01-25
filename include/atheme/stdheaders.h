/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Atheme Development Group (https://atheme.github.io/)
 *
 * This file is responsible for pulling in necessary standard system headers.
 * Each inclusion is commented with examples of the things they provide.
 */

#ifndef ATHEME_INC_STDINC_H
#define ATHEME_INC_STDINC_H 1

#include <atheme/sysconf.h>

#ifdef HAVE_STDDEF_H
// NULL, size_t
#  include <stddef.h>
#endif

#ifdef HAVE_STDARG_H
// va_list, va_arg(), va_start(), va_end()
#  include <stdarg.h>
#endif

#ifdef HAVE_SYS_TYPES_H
// Misc compiler/system types
#  include <sys/types.h>
#endif

#ifdef HAVE_INTTYPES_H
// Macros for printf(3) and scanf(3) for various integer widths
#  include <inttypes.h>
#endif

#ifdef HAVE_STDINT_H
// Fixed-width integers: {u,}int{8,16,32,64}_t
#  include <stdint.h>
#endif

#ifdef HAVE_CTYPE_H
// isalpha(), isalnum(), isdigit(), isspace(), ...
#  include <ctype.h>
#endif

#ifdef HAVE_DIRENT_H
// DT_*, opendir(), getdirentries(), readdir(), scandir(), closedir(), ...
#  include <dirent.h>
#endif

#ifdef HAVE_ERRNO_H
// errno ("extern int errno;" is not sufficient), E* (e.g. EAGAIN, EINTR, ...)
#  include <errno.h>
#endif

#ifdef HAVE_LIMITS_H
// CHAR_BIT, CHAR_MIN, CHAR_MAX, INT_MAX, PATH_MAX, ...
#  include <limits.h>
#endif

#ifdef HAVE_MATH_H
// floor(), pow(), ...
#  include <math.h>
#endif

#ifdef HAVE_NETDB_H
// struct addrinfo, getaddrinfo(), ...
#  include <netdb.h>
#endif

#ifdef HAVE_NETINET_IN_H
// struct in{6,}_addr, struct sockaddr_in{6,}, INET{6,}_ADDRSTRLEN, ...
#  include <netinet/in.h>
#endif

#ifdef HAVE_REGEX_H
// regex_t, regcomp(), regexec(), regerror(), regfree()
#  include <regex.h>
#endif

#ifdef HAVE_SIGNAL_H
// kill(), raise(), sigaction(), sigprocmask(), ...
#  include <signal.h>
#endif

#ifdef HAVE_STDBOOL_H
// bool, true, false
#  include <stdbool.h>
#endif

#ifdef HAVE_STDIO_H
// FILE*, {v,}{s,sn,f,}printf(), strerror(), perror(), ...
#  include <stdio.h>
#endif

#ifdef HAVE_STDLIB_H
// EXIT_*, calloc(), malloc(), realloc(), free(), getenv(), setenv(), ...
#  include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
// mem*(), str*()
#  include <string.h>
#endif

#ifdef HAVE_STRINGS_H
// {r,}index(), str{n,}casecmp(), ...
#  include <strings.h>
#endif

#ifdef HAVE_SYS_FILE_H
// flock(), LOCK_*, ...
#  include <sys/file.h>
#endif

#ifdef HAVE_SYS_RESOURCE_H
// getrlimit(), setrlimit(), RLIM_*, ...
#  include <sys/resource.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
// SHUT_*, struct mmsghdr, socket(), socketpair(), bind(), connect(), ...
#  include <sys/socket.h>
#endif

#ifdef HAVE_SYS_STAT_H
// S_I{F,S}*, S_*{USR,GRP,OTH}, {f,l,}stat{at,}(), {f,l,}chmod(), mkdir(), ...
#  include <sys/stat.h>
#endif

#ifdef HAVE_SYS_TIME_H
// gettimeofday(), futimes{at,}(), ...
#  include <sys/time.h>
#endif

#ifdef HAVE_SYS_WAIT_H
// W*, wait(), waitpid(), ...
#  include <sys/wait.h>
#endif

#ifdef HAVE_TIME_H
// time(), strftime(), localtime(), asctime(), timer_*(), ...
#  include <time.h>
#endif

#ifdef HAVE_UNISTD_H
// STD{IN,OUT,ERR}_FILENO, access(), close(), getpid(), lseek(), ...
#  include <unistd.h>
#endif

// MOWGLI_*, mowgli_*_t, mowgli_*()
#include <mowgli.h>

#endif /* !ATHEME_INC_STDINC_H */
