/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2020 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#include <atheme/attributes.h>      // ATHEME_FATTR_WUR
#include <atheme/bcrypt.h>          // atheme_eks_bf_testsuite_run()
#include <atheme/digest.h>          // digest_testsuite_run()
#include <atheme/i18n.h>            // _() (gettext)
#include <atheme/stdheaders.h>      // (everything else)

#include "benchmark.h"              // bench_print()
#include "selftests.h"              // self-declarations

bool ATHEME_FATTR_WUR
do_crypto_selftests(void)
{
	bool retval = true;

	if (! digest_testsuite_run())
	{
		(void) bench_print(_("The Digest API testsuite FAILED!"));
		retval = false;
	}
	else
		(void) bench_print(_("The Digest API testsuite passed."));

	if (! atheme_eks_bf_testsuite_run())
	{
		(void) bench_print(_("The bcrypt testsuite FAILED!"));
		retval = false;
	}
	else
		(void) bench_print(_("The bcrypt testsuite passed."));

	return retval;
}
