/*
 * Copyright (C) 2018 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Function, structure, and variable attribute macros.
 */

#ifndef ATHEME_INC_ATTRS_H
#define ATHEME_INC_ATTRS_H 1

// Compatibility with non-Clang compilers
#ifndef __has_attribute
#  define __has_attribute(attr) 0
#endif

/* Informs the compiler that the size of memory reachable by the pointer returned from this function is x (or x * y)
 * bytes, where x and y are positional function arguments (starting at 1). This aids static analysis and makes e.g.
 * __builtin_object_size(ptr) possible.
 *
 * Example:
 *   void *my_debug_malloc(const char *file, int line, size_t len) __attribute__((alloc_size(3)));
 *   #define DEBUG_MALLOC(len) my_debug_malloc(__FILE__, __LINE__, (len))
 */
#if __has_attribute(alloc_size)
#  define ATHEME_FATTR_ALLOC_SIZE(x)                    __attribute__((alloc_size((x))))
#  define ATHEME_FATTR_ALLOC_SIZE_PRODUCT(x, y)         __attribute__((alloc_size((x), (y))))
#else
#  define ATHEME_FATTR_ALLOC_SIZE(x)                    /* No 'alloc_size' function attribute support */
#  define ATHEME_FATTR_ALLOC_SIZE_PRODUCT(x, y)         /* No 'alloc_size' function attribute support */
#endif

// Diagnose when calls or references to this function are made
#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#  define ATHEME_FATTR_DEPRECATED                       __attribute__((deprecated))
#else
#  define ATHEME_FATTR_DEPRECATED                       /* No 'deprecated' function attribute support */
#endif

/* Informs the compiler to generate a diagnostic (type=="warning") or compilation failure (type=="error") when
 * calling this function with !cond; cond has access to its parameters by name. msg is the message to emit. Can
 * only be used with conditions that can be evaluated at compilation time.
 *
 * Example:
 *   void foo(void *ptr) __attribute__((diagnose_if(!ptr, "calling foo() with NULL 'ptr'", "error")));
 */
#if __has_attribute(diagnose_if)
#  define ATHEME_FATTR_DIAGNOSE_IF(cond, msg, type)     __attribute__((diagnose_if((cond), (msg), (type))))
#else
#  define ATHEME_FATTR_DIAGNOSE_IF(cond, msg, type)     /* No 'diagnose_if' function attribute support */
#endif

/* Inform the compiler that this function does not return to its caller. This aids some compiler optimisations,
 * such as not inserting return instructions and stack unwinding/validating logic into the end of the function, or
 * time spent evaluating whether a function can be tail-call-recursively optimised.
 *
 * Example:
 *   void my_debug_abort(const char *reason) __attribute__((noreturn));
 */
#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#  define ATHEME_FATTR_NORETURN                         __attribute__((noreturn))
#else
#  define ATHEME_FATTR_NORETURN                         /* No 'noreturn' function attribute support */
#endif

/* Have the compiler verify printf(3) or scanf(3) format tokens in the parameter position given by 'fmt' against the
 * function arguments starting at position 'start'. Also quenches warnings about passing 'fmt' itself to the
 * printf(3) or scanf(3) family of functions (because it is not a string literal). If the format string is available
 * but the list of arguments isn't (as in the case of e.g. stdarg(3) variadic functions), use 0 for 'start'.
 *
 * Examples:
 *   void log_message(int lvl, const char *fmt, ...) __attribute__((format(__printf__, 2, 3)));
 *   void vlog_message(int lvl, const char *fmt, va_args argv) __attribute__((format(__printf__, 2, 0)));
 */
#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#  define ATHEME_FATTR_PRINTF(fmt, start)               __attribute__((format(__printf__, (fmt), (start))))
#  define ATHEME_FATTR_SCANF(fmt, start)                __attribute__((format(__scanf__, (fmt), (start))))
#else
#  define ATHEME_FATTR_PRINTF(fmt, start)               /* No 'format' function attribute support */
#  define ATHEME_FATTR_SCANF(fmt, start)                /* No 'format' function attribute support */
#endif

/* Inform the compiler that the pointer returned by this function is never NULL.
 * This aids compiler optimisations and static analysis.
 */
#if __has_attribute(returns_nonnull)
#  define ATHEME_FATTR_RETURNS_NONNULL                  __attribute__((returns_nonnull))
#else
#  define ATHEME_FATTR_RETURNS_NONNULL                  /* No 'returns_nonnull' function attribute support */
#endif

// Diagnose if the return value of the function is unused by the caller (or under Clang, not explicitly discarded).
#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#  define ATHEME_FATTR_WUR                              __attribute__((warn_unused_result))
#else
#  define ATHEME_FATTR_WUR                              /* No 'warn_unused_result' function attribute support */
#endif

/* Inform the compiler that this function allocates memory; in particular, that the pointer it returns cannot
 * possibly alias or overlap another object that existed at the time the function was called; and also that if the
 * return value is a pointer to a structure, that any of the structure's pointer members also do not point to such
 * objects. This aids static analysis and some compiler optimisations.
 *
 * The warn_unused_result attribute is also present (if supported) because ignoring the return value could lead to
 * a memory leak.
 */
#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#  define ATHEME_FATTR_MALLOC                           __attribute__((malloc)) ATHEME_FATTR_WUR
#else
#  define ATHEME_FATTR_MALLOC                           /* No 'malloc' function attribute support */
#endif

// Inform the compiler that we don't want structure padding on this object
#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#  define ATHEME_SATTR_PACKED                           __attribute__((packed))
#else
#  define ATHEME_SATTR_PACKED                           /* No 'packed' structure attribute support */
#endif

// Inform the compiler that a variable may be unused (don't warn, whether it's used or not)
#if __has_attribute(maybe_unused)
#  define ATHEME_VATTR_MAYBE_UNUSED                     __attribute__((maybe_unused))
#else
#  define ATHEME_VATTR_MAYBE_UNUSED                     /* No 'maybe_unused' variable attribute support */
#endif

// Inform the compiler that a variable is unused (don't warn if it isn't used, and maybe warn if it is used)
#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#  define ATHEME_VATTR_UNUSED                           __attribute__((unused))
#else
#  define ATHEME_VATTR_UNUSED                           /* No 'unused' variable attribute support */
#endif

#endif /* !ATHEME_INC_ATTRS_H */
