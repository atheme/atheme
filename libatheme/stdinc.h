/*
 * Copyright (c) 2005 William Pitcock et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This is the header which includes all of the system stuff.
 *
 * $Id: stdinc.h 3377 2005-11-01 03:45:37Z nenolod $
 */

#ifndef STDINC_H
#define STDINC_H

/* I N C L U D E S */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <setjmp.h>
#include <sys/stat.h>
#include <ctype.h>
#include <regex.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#ifdef HAVE_LINK_H
#include <link.h>
#endif

/* socket stuff */
#ifndef _WIN32
# include <netdb.h>
# include <netinet/in.h>
# include <unistd.h>
# include <grp.h>
# include <sys/time.h>
# include <sys/wait.h>
# include <sys/resource.h>
# include <sys/socket.h>
# include <fcntl.h>
# include <arpa/inet.h>
#else
# include <windows.h>
# include <winsock.h>
# include <sys/timeb.h>
# include <direct.h>
# include <io.h>
# include <fcntl.h>
#endif

#include <sys/types.h>

#ifndef _WIN32
#include <libgen.h>
#endif
#include <dirent.h>

#ifndef _WIN32
typedef enum { ERROR = -1, FALSE, TRUE } l_boolean_t;
#else
typedef int l_boolean_t;
#define FALSE 0
#define TRUE 1
#endif

#undef boolean_t
#define boolean_t l_boolean_t

/* XXX these are all 32-bit types, not that I care. */
#ifdef _WIN32

#define itoa(num) r_itoa(num)
#define kill(n,m) 0

#define inet_ntop(a,b,c,d) strncpy( c, inet_ntoa( *((struct in_addr*)b) ), d);

/* XXX Microsoft headers are broken */
#define snprintf _snprintf
#endif

#endif
