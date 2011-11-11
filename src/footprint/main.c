/*
 * Copyright (c) 2005-2010 William Pitcock <nenolod@atheme.org>.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Initialization stub for libathemecore.
 */

#include "atheme.h"
#include "serno.h"

int main(int argc, char *argv[])
{
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

	printf("sizeof object_t: %zu B\n", sizeof(object_t));

	printf("\n* * *\n\n");

	printf("sizeof myentity_t: %zu B --> %zu KB\n", sizeof(myentity_t), (regusercount * sizeof(myentity_t)) / 1024);
	printf("sizeof myuser_t: %zu B --> %zu KB\n", sizeof(myuser_t), (regusercount * sizeof(myuser_t)) / 1024);
	printf("sizeof mychan_t: %zu B --> %zu KB\n", sizeof(mychan_t), (regchannelcount * sizeof(mychan_t)) / 1024);
	printf("sizeof mynick_t: %zu B --> %zu KB\n", sizeof(mynick_t), (regusercount * sizeof(mynick_t)) / 1024);

	printf("\n* * *\n\n");

	printf("sizeof user_t: %zu B --> %zu KB\n", sizeof(user_t), (usercount * sizeof(user_t)) / 1024);
	printf("sizeof channel_t: %zu B --> %zu KB\n", sizeof(channel_t), (channelcount * sizeof(channel_t)) / 1024);
	printf("sizeof chanuser_t: %zu B --> %zu KB\n", sizeof(chanuser_t), (membercount * sizeof(chanuser_t)) / 1024);

	printf("\n* * *\n\n");

	printf("sizeof kline_t: %zu B --> %zu KB\n", sizeof(kline_t), (klinecount * sizeof(kline_t)) / 1024);
	printf("sizeof xline_t: %zu B --> %zu KB\n", sizeof(xline_t), (xlinecount * sizeof(xline_t)) / 1024);
	printf("sizeof qline_t: %zu B --> %zu KB\n", sizeof(qline_t), (qlinecount * sizeof(qline_t)) / 1024);

	printf("\n* * *\n\n");

	printf("sizeof server_t: %zu B --> %zu KB\n", sizeof(server_t), (servercount * sizeof(server_t)) / 1024);

	return EXIT_SUCCESS;
}
