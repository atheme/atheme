/* OPENBSD ORIGINAL: lib/libc/string/explicit_bzero.c
 * $OpenBSD: explicit_bzero.c,v 1.1 2014/01/22 21:06:45 tedu Exp $
 *
 * Public domain. Written by Ted Unangst.
 */

#include <string.h>

#include "explicit_bzero.h"

/* We don't have explicit_bzero(3) [OpenBSD] or memset_s(3) [C11].
 *
 * Indirect memset(3) through a volatile function pointer should hopefully
 * prevent dead-store elimination removing the call.
 *
 * This may not work if Atheme IRC Services is built with Link Time
 * Optimisation, because the compiler may be able to prove (for a given
 * definition of proof) that the pointer always points to memset(3); LTO
 * lets the compiler analyse every compilation unit, not just this one.
 *
 * To hopefully prevent the compiler making assumptions about what it points
 * to, it is not isolated to this compilation unit. This file is part of a
 * library, so in theory a consumer of this library could modify this extern
 * variable to point to anything.
 *
 * Clang 6.0 with Thin LTO does not remove the call.
 * Other compilers are untested.
 */
void *(* volatile volatile_memset)(void *, int, size_t) = &memset;

void
explicit_bzero(void *const restrict p, const size_t n)
{
	(void) volatile_memset(p, 0x00, n);
}
