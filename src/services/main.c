/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2010 William Pitcock <nenolod@dereferenced.org>
 *
 * Initialization stub for libathemecore.
 */

#include <atheme.h>
#include <atheme/libathemecore.h>

int
main(int argc, char *argv[])
{
	if (! libathemecore_early_init())
		return EXIT_FAILURE;

	return atheme_main(argc, argv);
}
