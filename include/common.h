/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2006 Atheme Project (http://atheme.org/)
 *
 * Defines needed by multiple header files.
 */

#include "stdheaders.h"

#ifndef ATHEME_INC_COMMON_H
#define ATHEME_INC_COMMON_H 1

#define BUFSIZE			1024		/* maximum size of a buffer */
#define MAXMODES		4
#define MAX_IRC_OUTPUT_LINES	2000

/* lengths of various pieces of information (without NULL terminators) */
#define HOSTLEN			63		/* seems good enough */
#define NICKLEN			50
#define PASSLEN			288		/* 32 bytes salt + 1024 bits digest */
#define IDLEN			9
#define UIDLEN			16
#define CHANNELLEN		200
#define GROUPLEN		31
#define USERLEN			11
#define HOSTIPLEN		53
#define GECOSLEN		50
#define KEYLEN			23
#define EMAILLEN		254
#define MEMOLEN			299

#define MAXMSIGNORES		40

#undef DEBUG_BALLOC

#ifdef ATHEME_ENABLE_LARGE_NET
#  define HEAP_NODE             1024
#  define HEAP_CHANNEL          1024
#  define HEAP_CHANUSER         1024
#  define HEAP_USER             1024
#  define HEAP_GROUP		1024
#  define HEAP_SERVER           16
#  define HEAP_CHANACS          1024
#  define HEAP_GROUPACS		1024
#  define HASH_USER             65535
#  define HASH_CHANNEL          32768
#  define HASH_SERVER           128
#else /* ATHEME_ENABLE_LARGE_NET */
#  define HEAP_NODE             1024
#  define HEAP_CHANNEL          64
#  define HEAP_CHANUSER         128
#  define HEAP_USER             128
#  define HEAP_GROUP		128
#  define HEAP_SERVER           8
#  define HEAP_CHANACS          128
#  define HEAP_GROUPACS		128
#  define HASH_USER             1024
#  define HASH_CHANNEL          512
#  define HASH_SERVER           32
#endif /* !ATHEME_ENABLE_LARGE_NET */

#ifndef TIME_FORMAT
#define TIME_FORMAT		"%b %d %H:%M:%S %Y %z"
#endif

#define HASH_COMMAND		256
#define HASH_SMALL		32
#define HASH_ITRANS		128
#define HASH_TRANS		2048

#define CACHEFILE_HEAP_SIZE	32
#define CACHELINE_HEAP_SIZE	64

enum cmd_faultcode
{
	fault_needmoreparams	= 1,
	fault_badparams		= 2,
	fault_nosuch_source	= 3,
	fault_nosuch_target	= 4,
	fault_authfail		= 5,
	fault_noprivs		= 6,
	fault_nosuch_key	= 7,
	fault_alreadyexists	= 8,
	fault_toomany		= 9,
	fault_emailfail		= 10,
	fault_notverified	= 11,
	fault_nochange		= 12,
	fault_already_authed	= 13,
	fault_unimplemented	= 14,
	fault_badauthcookie	= 15,
	fault_internalerror	= 16,
};

/* Causes a warning if value is not of type (or compatible), returning value. */
#define ENSURE_TYPE(value, type) (true ? (value) : (type)0)

/* Returns the size of an array. */
#define ARRAY_SIZE(array) sizeof((array)) / sizeof(*(array))

/* Continue if an assertion fails. */
#define	continue_if_fail(x)						\
	if (!(x)) { 							\
		mowgli_log("critical: Assertion '%s' failed.", #x);	\
		continue;						\
	}

/* strshare.c - stringref management */
typedef const char *stringref;

void strshare_init(void);
stringref strshare_get(const char *str);
stringref strshare_ref(stringref str);
void strshare_unref(stringref str);

#endif /* !ATHEME_INC_COMMON_H */
