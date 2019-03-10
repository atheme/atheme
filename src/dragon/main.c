/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2013 William Pitcock <nenolod@dereferenced.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#include <atheme.h>
#include <atheme/libathemecore.h>

static struct timeval burstbegin;
static bool bursting = false;

void
bootstrap(void)
{
	if (me.name == NULL)
		me.name = sstrdup("services.dereferenced.org");

	if (me.numeric == NULL)
		me.numeric = sstrdup("00A");

	if (me.desc == NULL)
		me.desc = sstrdup("dragon ircd linking performance benchmark");

	servtree_update(NULL);

	slog(LG_INFO, "bootstrap: done, servtree root @%p", me.me);
}

bool
conf_check(void)
{
	return true;
}

void
build_world(void)
{
	int i;
	char userbuf[BUFSIZE];

	for (i = 100000; i > 0; i--)
	{
		snprintf(userbuf, sizeof userbuf, "User%d", i);
		user_add(userbuf, "user", "localhost", NULL, NULL, ircd->uses_uid ? uid_get() : NULL, "User", me.me, CURRTIME);
	}
}

void
burst_world(void)
{
	mowgli_node_t *n;

	slog(LG_INFO, "handshake complete, starting burst");

	s_time(&burstbegin);

	MOWGLI_ITER_FOREACH(n, me.me->userlist.head)
		introduce_nick(n->data);

	ping_sts();
	bursting = true;
}

void
phase_buildworld(void)
{
	struct timeval ts, te;

	slog(LG_INFO, "building world, please wait.");

	s_time(&ts);
	build_world();
	e_time(ts, &te);

	slog(LG_INFO, "world created in %d msec", tv2ms(&te));
}

static void
m_pong(struct sourceinfo *si, int parc, char *parv[])
{
	struct timeval te;

	if (!bursting)
	{
		burst_world();
		return;
	}

	e_time(burstbegin, &te);

	slog(LG_INFO, "burst took %d msec", tv2ms(&te));

	runflags |= RF_SHUTDOWN;
}

void
hijack_pong_handler(void)
{
	pcommand_delete("PONG");
	pcommand_add("PONG", m_pong, 1, MSRC_SERVER);
}

int
main(int argc, char *argv[])
{
	if (! libathemecore_early_init())
		return EXIT_FAILURE;

	atheme_bootstrap();
	atheme_init(argv[0], LOGDIR "/dbverify.log");
	atheme_setup();
	struct module *m;
	unsigned int errcnt;
	char *config_file = argv[1] != NULL ? argv[1] : "./dragon.conf";

	runflags = RF_LIVE;
	datadir = DATADIR;
	strict_mode = false;
	cold_start = true;

	slog(LG_INFO, "dragon: an ircd linking performance benchmark");
	slog(LG_INFO, "atheme.org, 2013");

	conf_parse(config_file);
	bootstrap();

	slog(LG_INFO, "link implementation: %s @%p", ircd->ircdname, ircd);

	hijack_pong_handler();

	mowgli_eventloop_synchronize(base_eventloop);
	CURRTIME = mowgli_eventloop_get_time(base_eventloop);

	phase_buildworld();
	uplink_connect();

	slog(LG_INFO, "uplink: %s @%p", curr_uplink->name, curr_uplink);

	io_loop();

	return EXIT_SUCCESS;
}
