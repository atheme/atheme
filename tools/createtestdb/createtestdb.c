/*-
 * Copyright (c) 2008 Jilles Tjoelker
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * make createtestdb
 * ./createtestdb 500000 >atheme.db
 * 
 * then start atheme-services with this atheme.db
 */

#include	<stdio.h>
#include	<time.h>
#include	<stdlib.h>

#define MUFLAGS 0x100 /* MU_CRYPTPASS */

int
main(int argc, char *argv[])
{
	int count, i;
	char nick[20];
	time_t now;

	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s count\n", argv[0]);
		return 1;
	}

	count = atoi(argv[1]);

	now = time(NULL);

	printf("# Test database with %d accounts\n", count);
	printf("DBV 4\n");
	printf("CF +vVhHoOtsriRfAb\n");
	for (i = 0; i < count; i++)
	{
		snprintf(nick, sizeof nick, "a%010d", i);
		printf("MU %s * noemail %lu %lu 0 0 0 %d\n", nick,
				(unsigned long)now,
				(unsigned long)now, MUFLAGS);
		printf("MN %s %s %lu %lu\n", nick, nick, (unsigned long)now,
				(unsigned long)now);
	}

	return 0;
}
