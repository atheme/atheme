/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Undernet base64 routine.
 *
 * $Id: ubase64.c 2497 2005-10-01 04:35:25Z nenolod $
 */

#include "atheme.h"

/*
 * Rewritten 07/17/05 by nenolod, due to legal concerns
 * of using GPL'ed ircu code in the main tree, raised
 * by Zoot (over at srvx, who have also rewritten their
 * base64 implementation for srvx version 2).
 *
 * Run may be a GPL zealot, that's really ok, because
 * our implementation is faster and more secure...
 */

static const char ub64_alphabet[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789[]";

const char *uinttobase64(char *buf, uint64_t v, int64_t count)
{
	buf[count] = '\0';

	do
	{
		buf[--count] = ub64_alphabet[v & 63];
	}
	while (v >>= 6 && count >= 0);

	return buf;
}
