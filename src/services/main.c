/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 William Pitcock <nenolod@atheme.org>
 *
 * Initialization stub for libathemecore.
 */

#include "atheme.h"
#include "libathemecore.h"

#ifdef HAVE_LIBSODIUM
#  include <sodium/core.h>
#endif /* HAVE_LIBSODIUM */

int
main(int argc, char *argv[])
{
#ifdef HAVE_LIBSODIUM
	if (sodium_init() == -1)
	{
		(void) fprintf(stderr, "Error: sodium_init() failed!\n");
		return EXIT_FAILURE;
	}
#endif /* HAVE_LIBSODIUM */

	return atheme_main(argc, argv);
}
