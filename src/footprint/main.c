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

	unsigned int usercount = 0, channelcount = 0, membercount = 0,
		klinecount = 0, qlinecount = 0, xlinecount = 0, regchannelcount = 0,
		servercount = 0, regusercount = 0;
	unsigned int i;

	/* make up some statistics */
	srand(time(NULL));

	servercount = (rand() % 30) + 1;

	for (i = 0; i < servercount; i++)
		usercount += ((rand() % 6000) + 1);

	channelcount = usercount / servercount;
	membercount = (usercount + channelcount) * ((rand() % 3) + 1);
	regchannelcount = channelcount * 0.65;
	regusercount = usercount * 0.65;

	/* 5% of users are probably misbehaving in some way... */
	klinecount = xlinecount = qlinecount = (usercount * 0.05);

	printf("footprint for atheme %s (%s)\n", PACKAGE_VERSION, SERNO);

	printf("\n* * *\n\n");

	printf("%u servers\n", servercount);
	printf("%u users\n", usercount);
	printf("%u registered users\n", regusercount);
	printf("%u channels\n", channelcount);
	printf("%u registered channels\n", regchannelcount);
	printf("%u memberships\n", membercount);
	printf("%u klines / xlines / qlines\n", klinecount);

	printf("\n* * *\n\n");

	printf("sizeof object_t: %zu B\n", sizeof(struct atheme_object));

	printf("\n* * *\n\n");

	printf("sizeof myentity_t: %zu B --> %zu KB\n", sizeof(struct myentity), (regusercount * sizeof(struct myentity)) / 1024);
	printf("sizeof myuser_t: %zu B --> %zu KB\n", sizeof(struct myuser), (regusercount * sizeof(struct myuser)) / 1024);
	printf("sizeof mychan_t: %zu B --> %zu KB\n", sizeof(struct mychan), (regchannelcount * sizeof(struct mychan)) / 1024);
	printf("sizeof mynick_t: %zu B --> %zu KB\n", sizeof(struct mynick), (regusercount * sizeof(struct mynick)) / 1024);

	printf("\n* * *\n\n");

	printf("sizeof user_t: %zu B --> %zu KB\n", sizeof(struct user), (usercount * sizeof(struct user)) / 1024);
	printf("sizeof channel_t: %zu B --> %zu KB\n", sizeof(struct channel), (channelcount * sizeof(struct channel)) / 1024);
	printf("sizeof chanuser_t: %zu B --> %zu KB\n", sizeof(struct chanuser), (membercount * sizeof(struct chanuser)) / 1024);

	printf("\n* * *\n\n");

	printf("sizeof kline_t: %zu B --> %zu KB\n", sizeof(struct kline), (klinecount * sizeof(struct kline)) / 1024);
	printf("sizeof xline_t: %zu B --> %zu KB\n", sizeof(struct xline), (xlinecount * sizeof(struct xline)) / 1024);
	printf("sizeof qline_t: %zu B --> %zu KB\n", sizeof(struct qline), (qlinecount * sizeof(struct qline)) / 1024);

	printf("\n* * *\n\n");

	printf("sizeof server_t: %zu B --> %zu KB\n", sizeof(struct server), (servercount * sizeof(struct server)) / 1024);

	return EXIT_SUCCESS;
}
