/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * Includes most headers usually needed.
 *
 * $Id: atheme.h 7481 2007-01-14 08:19:09Z nenolod $
 */

#ifndef ATHEME_H
#define ATHEME_H

/* *INDENT-OFF* */

#ifdef _WIN32                   /* Windows */
#       ifdef I_AM_A_MODULE
#               define DLE __declspec (dllimport)
#               define E extern DLE
#       else
#               define DLE __declspec (dllexport)
#               define E extern DLE
#       endif
#else                           /* POSIX */
#       define E extern
#       define DLE
#endif

#define RF_LIVE         0x00000001      /* don't fork  */
#define RF_SHUTDOWN     0x00000002      /* shut down   */
#define RF_STARTING     0x00000004      /* starting up */
#define RF_RESTART      0x00000008      /* restart     */
#define RF_REHASHING    0x00000010      /* rehashing   */

#define LG_NONE         0x00000001      /* don't log                */
#define LG_INFO         0x00000002      /* log general info         */
#define LG_ERROR        0x00000004      /* log real important stuff */
#define LG_IOERROR      0x00000008      /* log I/O errors. */
#define LG_DEBUG        0x00000010      /* log debugging stuff      */

#define BUFSIZE  1024            /* maximum size of a buffer */
#define HOSTLEN  64		 /* seems good enough */
#define MAX_EVENTS	1024	 /* that's enough events, really! */

#define HEAP_NODE       1024

#include "sysconf.h"
#include "stdinc.h"
#include "dlink.h"
#include "event.h"
#include "balloc.h"
#include "connection.h"
#include "sockio.h"
#include "hook.h"
#include "linker.h"
#include "atheme_string.h"
#include "atheme_memory.h"
#include "datastream.h"
#include "common.h"
#include "dictionary.h"
#include "servers.h"
#include "channels.h"
#include "module.h"
#include "crypto.h"
#include "culture.h"
#include "xmlrpc.h"
#include "base64.h"
#include "md5.h"
#include "sasl.h"
#include "match.h"
#include "sysconf.h"
#include "account.h"
#include "tools.h"
#include "confparse.h"
#include "global.h"
#include "flags.h"
#include "metadata.h"
#include "phandler.h"
#include "servtree.h"
#include "services.h"
#include "commandtree.h"
#include "users.h"
#include "sourceinfo.h"
#include "authcookie.h"
#include "privs.h"
#include "object.h"

#ifdef _WIN32

/* Windows + Module -> needs these to be declared before using them */
#ifdef I_AM_A_MODULE
void _modinit(module_t *m);
void _moddeinit(void);
#endif

/* Windows has an extremely stupid gethostbyname() function. Oof! */
#define gethostbyname(a) gethostbyname_layer(a)

#endif

#define CURRTIME claro_state.currtime

typedef struct claro_state_ {
	uint32_t node;
	uint32_t event;
	time_t currtime;
	uint16_t maxfd;
} claro_state_t;

E claro_state_t claro_state;

E int runflags;

/*
 * Performs a soft assertion. If the assertion fails, we wallops() and log.
 */
#ifdef __GNUC__
#define soft_assert(x)								\
	if (!(x)) { 								\
		slog(LG_INFO, "%s(%d) [%s]: critical: Assertion '%s' failed.",	\
			__FILE__, __LINE__, __PRETTY_FUNCTION__, #x);		\
		wallops("%s(%d) [%s]: critical: Assertion '%s' failed.",	\
			__FILE__, __LINE__, __PRETTY_FUNCTION__, #x);		\
	}
#else
#define soft_assert(x)								\
	if (!(x)) { 								\
		slog(LG_INFO, "%s(%d): critical: Assertion '%s' failed.",	\
			__FILE__, __LINE__, #x);				\
		wallops("%s(%d): critical: Assertion '%s' failed.",		\
			__FILE__, __LINE__, #x);				\
	}
#endif

/*
 * Same as soft_assert, but returns if an assertion fails.
 */
#ifdef __GNUC__
#define return_if_fail(x)							\
	if (!(x)) { 								\
		slog(LG_INFO, "%s(%d) [%s]: critical: Assertion '%s' failed.",	\
			__FILE__, __LINE__, __PRETTY_FUNCTION__, #x);		\
		wallops("%s(%d) [%s]: critical: Assertion '%s' failed.",	\
			__FILE__, __LINE__, __PRETTY_FUNCTION__, #x);		\
		return;								\
	}
#else
#define return_if_fail(x)							\
	if (!(x)) { 								\
		slog(LG_INFO, "%s(%d): critical: Assertion '%s' failed.",	\
			__FILE__, __LINE__, #x);				\
		wallops("%s(%d): critical: Assertion '%s' failed.",		\
			__FILE__, __LINE__, #x);				\
		return;								\
	}
#endif

/*
 * Same as soft_assert, but returns a given value if an assertion fails.
 */
#ifdef __GNUC__
#define return_val_if_fail(x, y)						\
	if (!(x)) { 								\
		slog(LG_INFO, "%s(%d) [%s]: critical: Assertion '%s' failed.",	\
			__FILE__, __LINE__, __PRETTY_FUNCTION__, #x);		\
		wallops("%s(%d) [%s]: critical: Assertion '%s' failed.",	\
			__FILE__, __LINE__, __PRETTY_FUNCTION__, #x);		\
		return (y);							\
	}
#else
#define return_val_if_fail(x, y)						\
	if (!(x)) { 								\
		slog(LG_INFO, "%s(%d): critical: Assertion '%s' failed.",	\
			__FILE__, __LINE__, #x);				\
		wallops("%s(%d): critical: Assertion '%s' failed.",		\
			__FILE__, __LINE__, #x);				\
		return (y);							\
	}
#endif

#endif /* ATHEME_H */
