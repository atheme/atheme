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
#include	<string.h>

#define		BUFSIZE 1024
#define		SID "007"

static char new_uid[9];
static unsigned int uindex = 9;

void init_uid(void)
{
	unsigned int i;
	char buf[BUFSIZE];

	memset(new_uid, 0, sizeof(new_uid));
	memcpy(new_uid, SID, strlen(SID));

	for (i = 0; i < strlen(SID); i++)
		if (new_uid[i] == '\0')
			new_uid[i] = 'A';

	for (i = strlen(SID); i < uindex; i++)
		new_uid[i] = 'A';
}

static void add_one_to_uid(unsigned int i)
{
	if (i != strlen(SID))	/* Not reached server SID portion yet? */
	{
		if (new_uid[i] == 'Z')
			new_uid[i] = '0';
		else if (new_uid[i] == '9')
		{
			new_uid[i] = 'A';
			add_one_to_uid(i - 1);
		}
		else
			new_uid[i] = new_uid[i] + 1;
	}
	else
	{
		if (new_uid[i] == 'Z')
			for (i = strlen(SID); i < 9; i++)
				new_uid[i] = 'A';
		else
			new_uid[i] = new_uid[i] + 1;
	}
}

const char *uid_get(void)
{
	add_one_to_uid(uindex - 1);	/* index from 0 */
	return (new_uid);
}

int
do_burst_ts5(int count, time_t now)
{
	int i;
	char nick[20];

	printf("PASS linkit TS\r\n");
	printf("CAPAB :QS EX IE KLN UNKLN ENCAP TB SERVICES EUID EOPMOD MLOCK\r\n");
	printf("SERVER irc.uplink.com 1 :Test file\r\n");
	printf("SVINFO 5 3 0 :%lu\r\n", (unsigned long) now);

	for (i = 0; i < count; i++)
	{
		snprintf(nick, sizeof nick, "a%010d", i);
		printf("NICK %s 1 %lu +i ~Guest moo.cows.go.moo irc.uplink.com :Grazing cow #%ld\r\n",
				nick, (unsigned long) now, i);
	}

	printf(":irc.uplink.com PONG irc.uplink.com :irc.uplink.com\r\n");

	return 0;
}

int
do_burst_ts6(int count, time_t now)
{
	int i;
	char nick[20];

	printf("PASS linkit TS 6 :%s\r\n", SID);
	printf("CAPAB :QS EX IE KLN UNKLN ENCAP TB SERVICES EUID EOPMOD MLOCK\r\n");
	printf("SERVER irc.uplink.com 1 :Test file\r\n");
	printf("SVINFO 6 3 0 :%lu\r\n", (unsigned long) now);

	for (i = 0; i < count; i++)
	{
		snprintf(nick, sizeof nick, "a%010d", i);
		printf(":%s UID %s 1 %lu +i ~Guest moo.cows.go.moo 0 %s :Grazing cow #%ld\r\n",
				SID, nick, (unsigned long) now, uid_get(), i);
	}

	printf(":%s PONG %s :%s\r\n", SID, SID, SID);

	return 0;
}

int
main(int argc, char *argv[])
{
	int count;
	time_t now;

	init_uid();

	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s count\n", argv[0]);
		return 1;
	}

	count = atoi(argv[1]);
	now = time(NULL);

	if (argc > 2)
		return do_burst_ts5(count, now);

	return do_burst_ts6(count, now);
}
