/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Function, structure, and variable attribute macros.
 */

#ifndef ATHEME_INC_ATTRIBUTES_H
#define ATHEME_INC_ATTRIBUTES_H 1

/* Woe is me for spending many hours looking up in the GCC documentation
 * which old versions introduced which attributes.
 *
 * <https://gcc.gnu.org/onlinedocs/gcc-3.4.0/gcc/>
 * <https://gcc.gnu.org/onlinedocs/gcc-4.3.0/gcc/>
 * <https://gcc.gnu.org/onlinedocs/gcc-4.9.0/gcc/>
 *
 * We don't test for the GCC patchlevel. If e.g. v4.9.1 introduced an
 * attribute that v4.9.0 lacks, too bad; use >= v5 instead if you want the
 * attribute that badly.
 *
 * Clang >= v3.7 and GCC >= v5 support the __has_attribute() function-like
 * macro used below, so we just use that instead. This is why there are no
 * links above to the GCC documentation for newer versions.
 *
 *   -- amdj
 */

#ifdef __has_attribute
#  if __has_attribute(__alloc_size__)
#    define ATHEME_ATTR_HAS_ALLOC_SIZE                  1
#  endif
#  if __has_attribute(__deprecated__)
#    define ATHEME_ATTR_HAS_DEPRECATED                  1
#  endif
#  if __has_attribute(__diagnose_if__)
#    define ATHEME_ATTR_HAS_DIAGNOSE_IF                 1
#  endif
#  if __has_attribute(__fallthrough__)
#    define ATHEME_ATTR_HAS_FALLTHROUGH                 1
#  endif
#  if __has_attribute(__format__)
#    define ATHEME_ATTR_HAS_FORMAT                      1
#  endif
#  if __has_attribute(__malloc__)
#    define ATHEME_ATTR_HAS_MALLOC                      1
#  endif
#  if __has_attribute(__maybe_unused__)
#    define ATHEME_ATTR_HAS_MAYBE_UNUSED                1
#  endif
#  if __has_attribute(__noreturn__)
#    define ATHEME_ATTR_HAS_NORETURN                    1
#  endif
#  if __has_attribute(__packed__)
#    define ATHEME_ATTR_HAS_PACKED                      1
#  endif
#  if __has_attribute(__returns_nonnull__)
#    define ATHEME_ATTR_HAS_RETURNS_NONNULL             1
#  endif
#  if __has_attribute(__unused__)
#    define ATHEME_ATTR_HAS_UNUSED                      1
#  endif
#  if __has_attribute(__warn_unused_result__)
#    define ATHEME_ATTR_HAS_WARN_UNUSED_RESULT          1
#  endif
#else
#  if !defined(__clang__) && defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
#    if ((__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 4)))
#      define ATHEME_ATTR_HAS_DEPRECATED                1
#      define ATHEME_ATTR_HAS_FORMAT                    1
#      define ATHEME_ATTR_HAS_MALLOC                    1
#      define ATHEME_ATTR_HAS_NORETURN                  1
#      define ATHEME_ATTR_HAS_PACKED                    1
#      define ATHEME_ATTR_HAS_UNUSED                    1
#      define ATHEME_ATTR_HAS_WARN_UNUSED_RESULT        1
#    endif
#    if ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 3)))
#      define ATHEME_ATTR_HAS_ALLOC_SIZE                1
#    endif
#    if ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 9)))
#      define ATHEME_ATTR_HAS_RETURNS_NONNULL           1
#    endif
#  endif
#endif



/* Informs the compiler that the size of memory reachable by the pointer returned from this function is x (or x * y)
 * bytes, where x and y are positional function arguments (starting at 1). This aids static analysis and makes e.g.
 * __builtin_object_size(ptr) possible.
 *
 * Example:
 *   void *my_debug_malloc(const char *file, int line, size_t len) __attribute__((alloc_size(3)));
 *   #define DEBUG_MALLOC(len) my_debug_malloc(__FILE__, __LINE__, (len))
 */
#ifdef ATHEME_ATTR_HAS_ALLOC_SIZE
#  define ATHEME_FATTR_ALLOC_SIZE(x)                    __attribute__((__alloc_size__((x))))
#  define ATHEME_FATTR_ALLOC_SIZE_PRODUCT(x, y)         __attribute__((__alloc_size__((x), (y))))
#else
#  define ATHEME_FATTR_ALLOC_SIZE(x)                    /* No 'alloc_size' function attribute support */
#  define ATHEME_FATTR_ALLOC_SIZE_PRODUCT(x, y)         /* No 'alloc_size' function attribute support */
#endif

// Diagnose when calls or references to this function are made
#ifdef ATHEME_ATTR_HAS_DEPRECATED
#  define ATHEME_FATTR_DEPRECATED                       __attribute__((__deprecated__))
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
#ifdef ATHEME_ATTR_HAS_DIAGNOSE_IF
#  define ATHEME_FATTR_DIAGNOSE_IF(cond, msg, type)     __attribute__((__diagnose_if__((cond), (msg), (type))))
#else
#  define ATHEME_FATTR_DIAGNOSE_IF(cond, msg, type)     /* No 'diagnose_if' function attribute support */
#endif

/* Prevent the compiler from warning about fall-through in switch() case statements. This one is a little bit
 * unique, in that its name is shorter because it is intended to be used within a block of code multiple times,
 * and so saves a lot of typing and eyesore. Its name is still prefixed with ATHEME_ however, to prevent any
 * unintended collision with a FALLTHROUGH macro declared in the header files of any library we end up using.
 */
#ifdef ATHEME_ATTR_HAS_FALLTHROUGH
#  define ATHEME_FALLTHROUGH                            __attribute__((__fallthrough__))
#else
#  define ATHEME_FALLTHROUGH                            /* No 'fallthrough' statement attribute support */
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
#ifdef ATHEME_ATTR_HAS_FORMAT
#  define ATHEME_FATTR_PRINTF(fmt, start)               __attribute__((__format__(__printf__, (fmt), (start))))
#  define ATHEME_FATTR_SCANF(fmt, start)                __attribute__((__format__(__scanf__, (fmt), (start))))
#else
#  define ATHEME_FATTR_PRINTF(fmt, start)               /* No 'format' function attribute support */
#  define ATHEME_FATTR_SCANF(fmt, start)                /* No 'format' function attribute support */
#endif

/* Inform the compiler that this function allocates memory; in particular, that the pointer it returns cannot
 * possibly alias or overlap another object that existed at the time the function was called; and also that if the
 * return value is a pointer to a structure, that any of the structure's pointer members also do not point to such
 * objects. This aids static analysis and some compiler optimisations.
 *
 * The warn_unused_result attribute is also present (if supported) because ignoring the return value could lead to
 * a memory leak.
 */
#ifdef ATHEME_ATTR_HAS_MALLOC
#  ifdef ATHEME_ATTR_HAS_WARN_UNUSED_RESULT
#    define ATHEME_FATTR_MALLOC                         __attribute__((__malloc__, __warn_unused_result__))
#  else
#    define ATHEME_FATTR_MALLOC                         __attribute__((__malloc__))
#  endif
#else
#  define ATHEME_FATTR_MALLOC                           /* No 'malloc' function attribute support */
#endif

/* Inform the compiler that a variable may be unused (don't warn, whether it's used or not). This is distinct
 * from __attribute__((unused)) because some compilers (e.g. Clang) will warn you if you use such a variable.
 */
#ifdef ATHEME_ATTR_HAS_MAYBE_UNUSED
#  define ATHEME_VATTR_MAYBE_UNUSED                     __attribute__((__maybe_unused__))
#else
#  define ATHEME_VATTR_MAYBE_UNUSED                     /* No 'maybe_unused' variable attribute support */
#endif

/* Inform the compiler that this function does not return to its caller. This aids some compiler optimisations,
 * such as not inserting return instructions and stack unwinding/validating logic into the end of the function, or
 * time spent evaluating whether a function can be tail-call-recursively optimised. It also aids static analysis
 * because an analyser can assume this function is an 'assertion handler' (i.e. that it terminates the program),
 * which lets it assume that certain code paths are unreachable.
 *
 * Example:
 *   void my_debug_abort(const char *reason) __attribute__((noreturn));
 */
#ifdef ATHEME_ATTR_HAS_NORETURN
#  define ATHEME_FATTR_NORETURN                         __attribute__((__noreturn__))
#else
#  define ATHEME_FATTR_NORETURN                         /* No 'noreturn' function attribute support */
#endif

// Inform the compiler that we don't want structure padding on this object
#ifdef ATHEME_ATTR_HAS_PACKED
#  define ATHEME_SATTR_PACKED                           __attribute__((__packed__))
#else
#  define ATHEME_SATTR_PACKED                           /* No 'packed' structure attribute support */
#endif

/* Inform the compiler that the pointer returned by this function is never NULL.
 * This aids compiler optimisations and static analysis.
 */
#ifdef ATHEME_ATTR_HAS_RETURNS_NONNULL
#  define ATHEME_FATTR_RETURNS_NONNULL                  __attribute__((__returns_nonnull__))
#else
#  define ATHEME_FATTR_RETURNS_NONNULL                  /* No 'returns_nonnull' function attribute support */
#endif

// Inform the compiler that a variable is unused (don't warn if it isn't used, and maybe warn if it is used)
#ifdef ATHEME_ATTR_HAS_UNUSED
#  define ATHEME_VATTR_UNUSED                           __attribute__((__unused__))
#else
#  define ATHEME_VATTR_UNUSED                           /* No 'unused' variable attribute support */
#endif

// Diagnose if the return value of the function is unused by the caller (or under Clang, not explicitly discarded).
#ifdef ATHEME_ATTR_HAS_WARN_UNUSED_RESULT
#  define ATHEME_FATTR_WUR                              __attribute__((__warn_unused_result__))
#else
#  define ATHEME_FATTR_WUR                              /* No 'warn_unused_result' function attribute support */
#endif

#endif /* !ATHEME_INC_ATTRIBUTES_H */
