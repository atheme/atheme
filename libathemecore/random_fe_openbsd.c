/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Frontend routines for the random interface (OpenBSD backend).
 */

#ifndef ATHEME_RANDOM_FRONTEND_C
#  error "Do not compile me directly; compile random_frontend.c instead"
#endif /* !ATHEME_RANDOM_FRONTEND_C */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "atheme_random.h"

uint32_t
atheme_random(void)
{
	return arc4random();
}

uint32_t
atheme_random_uniform(const uint32_t bound)
{
	return arc4random_uniform(bound);
}

void
atheme_random_buf(void *const restrict out, const size_t len)
{
	(void) arc4random_buf(out, len);
}

const char *
random_get_frontend_info(void)
{
	return "OpenBSD arc4random(3)";
}
