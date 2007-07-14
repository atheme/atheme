/*
 * Copyright (c) 2005 William Pitcock et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This is the header which includes all of the system stuff.
 *
 * $Id: stdinc.h 8079 2007-04-02 17:37:39Z nenolod $
 */

#ifndef STDINC_H
#define STDINC_H

/* I N C L U D E S */
#include <mowgli.h>

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
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <grp.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include <sys/types.h>

#include <libgen.h>
#include <dirent.h>

typedef enum { FALSE, TRUE } l_boolean_t;

#undef boolean_t
#define boolean_t l_boolean_t

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
