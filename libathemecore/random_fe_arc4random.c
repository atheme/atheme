/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Aaron M. D. Jones <me@aaronmdjones.net>
 *
 * Frontend routines for the random interface (arc4random(3) backend).
 */

#ifndef ATHEME_LAC_RANDOM_FRONTEND_C
#  error "Do not compile me directly; compile random_frontend.c instead"
#endif /* !ATHEME_LAC_RANDOM_FRONTEND_C */

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

bool ATHEME_FATTR_WUR
libathemecore_random_early_init(void)
{
	(void) atheme_random();

	return true;
}

const char *
random_get_frontend_info(void)
{
	return "Secure arc4random(3)";
}
