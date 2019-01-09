/*
 * Copyright (c) 2005-2010 William Pitcock <nenolod@atheme.org>.
 * Rights to this code are as documented in doc/LICENSE.
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
