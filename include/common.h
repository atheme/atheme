/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Defines needed by multiple header files.
 *
 */

#ifndef COMMON_H
#define COMMON_H

/* D E F I N E S */
#define BUFSIZE			1024		/* maximum size of a buffer */
#define MAXMODES		4
#define MAX_EVENTS		1024		/* that's enough events, really! */
#define MAX_IRC_OUTPUT_LINES	2000

/* lengths of buffers (string length is 1 less) */
#define HOSTLEN			64		/* seems good enough */
#define NICKLEN			32
#define PASSLEN			80
#define IDLEN			10
#define CHANNELLEN		201
#define USERLEN			12
#define HOSTIPLEN		54
#define GECOSLEN		51
#define KEYLEN			24
#define EMAILLEN		120
#define MEMOLEN			300

#define MAXMSIGNORES		40

#undef DEBUG_BALLOC

#ifdef LARGE_NETWORK
#define HEAP_NODE		1024
#define HEAP_CHANNEL		1024
#define HEAP_CHANUSER		1024
#define HEAP_USER		1024
#define HEAP_SERVER		16
#define HEAP_CHANACS		1024
#define HASH_USER		65535
#define HASH_CHANNEL		32768
#define HASH_SERVER		128
#else
#define HEAP_NODE		1024
#define HEAP_CHANNEL		64
#define HEAP_CHANUSER		128
#define HEAP_USER		128
#define HEAP_SERVER		8
#define HEAP_CHANACS		128
#define HASH_USER		1024
#define HASH_CHANNEL		512
#define HASH_SERVER		32
#endif

#define HASH_COMMAND		256
#define HASH_SMALL		32
#define HASH_ITRANS		128
#define HASH_TRANS		2048

#define CACHEFILE_HEAP_SIZE	32
#define CACHELINE_HEAP_SIZE	64

/* Make it possible to use pointers to these types everywhere
 * (for structures used in multiple header files) */
typedef struct user_ user_t;

typedef struct server_ server_t;

typedef struct channel_ channel_t;
typedef struct chanuser_ chanuser_t;
typedef struct chanban_ chanban_t;

typedef struct operclass_ operclass_t;
typedef struct soper_ soper_t;
typedef struct myuser_ myuser_t;
typedef struct mynick_ mynick_t;
typedef struct mychan_ mychan_t;

typedef struct service_ service_t;

typedef struct sourceinfo_ sourceinfo_t;

typedef struct _configfile config_file_t;
typedef struct _configentry config_entry_t;

enum faultcode_
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
	fault_badauthcookie	= 15
};

typedef enum faultcode_ faultcode_t;

#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#define PRINTFLIKE(fmtarg, firstvararg) \
	__attribute__((__format__ (__printf__, fmtarg, firstvararg)))
#define SCANFLIKE(fmtarg, firstvararg) \
	__attribute__((__format__ (__scanf__, fmtarg, firstvararg)))
#define DEPRECATED \
	__attribute__((deprecated))
#else
#define PRINTFLIKE(fmtarg, firstvararg)
#define SCANFLIKE(fmtarg, firstvararg)
#define DEPRECATED
#endif /* defined(__INTEL_COMPILER) || defined(__GNUC__) */

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

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs ts=8 sw=8 noexpandtab
 */
