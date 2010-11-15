/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_stdinc.h: Pulls in the base system includes for libmowgli.
 *
 * Copyright (c) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice is present in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __MOWGLI_STDINC_H__
#define __MOWGLI_STDINC_H__

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
# include <libgen.h>
# include <dirent.h>
#else
# include <windows.h>
# include <winsock.h>
# include <sys/timeb.h>
# include <direct.h>
# include <io.h>
# include <fcntl.h>
#endif

#include <sys/types.h>

#ifdef _MSC_VER
# pragma warning (disable: 4996)
#endif

#ifdef FALSE
# undef FALSE
#endif

#ifdef TRUE
# undef TRUE
#endif

typedef enum { FALSE, TRUE } mowgli_boolean_t;

/* Macros for min/max.  */
#ifndef MIN
# define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#ifndef MAX
# define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#endif
