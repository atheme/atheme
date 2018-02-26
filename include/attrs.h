/*
 * Copyright (C) 2018 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Function, structure, and variable attribute macros.
 */

#ifndef ATHEME_INC_ATTRS_H
#define ATHEME_INC_ATTRS_H 1

#if defined(__GNUC__) || defined(__INTEL_COMPILER)

#define ATHEME_FATTR_DEPRECATED         __attribute__((deprecated))
#define ATHEME_FATTR_MALLOC             __attribute__((malloc))
#define ATHEME_FATTR_NORETURN           __attribute__((noreturn))
#define ATHEME_FATTR_PRINTF(fmt, start) __attribute__((format(__printf__, fmt, start)))
#define ATHEME_FATTR_SCANF(fmt, start)  __attribute__((format(__scanf__, fmt, start)))
#define ATHEME_FATTR_WUR                __attribute__((warn_unused_result))

#define ATHEME_SATTR_PACKED             __attribute__((packed))

#define ATHEME_VATTR_ALIGNED(alignment) __attribute__((aligned(alignment)))
#define ATHEME_VATTR_UNUSED             __attribute__((unused))

#else

#define ATHEME_FATTR_DEPRECATED         /* No function attribute support */
#define ATHEME_FATTR_MALLOC             /* No function attribute support */
#define ATHEME_FATTR_NORETURN           /* No function attribute support */
#define ATHEME_FATTR_PRINTF(fmt, start) /* No function attribute support */
#define ATHEME_FATTR_SCANF(fmt, start)  /* No function attribute support */
#define ATHEME_FATTR_WUR                /* No function attribute support */

#define ATHEME_SATTR_PACKED             /* No structure attribute support */

#define ATHEME_VATTR_ALIGNED(alignment) /* No variable attribute support */
#define ATHEME_VATTR_UNUSED             /* No variable attribute support */

#endif /* (__GNUC__ || __INTEL_COMPILER) */

#endif /* !ATHEME_INC_ATTRS_H */
