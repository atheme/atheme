/*
 * Copyright (c) 2013 William Pitcock <nenolod@dereferenced.org>.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "atheme.h"
#include "libathemecore.h"
#include "uplink.h"
#include "pmodule.h"
#include "conf.h"

static struct timeval burstbegin;
static bool bursting = false;

void bootstrap(void)
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

bool conf_check(void)
{
	return true;
}

void build_world(void)
{
	int i;
	char userbuf[BUFSIZE];

	for (i = 100000; i > 0; i--)
	{
		snprintf(userbuf, sizeof userbuf, "User%d", i);
		user_add(userbuf, "user", "localhost", NULL, NULL, ircd->uses_uid ? uid_get() : NULL, "User", me.me, CURRTIME);
	}
}

void burst_world(void)
{
	mowgli_node_t *n;

	slog(LG_INFO, "handshake complete, starting burst");

	s_time(&burstbegin);

	MOWGLI_ITER_FOREACH(n, me.me->userlist.head)
		introduce_nick(n->data);

	ping_sts();
	bursting = true;
}

void phase_buildworld(void)
{
	struct timeval ts, te;

	slog(LG_INFO, "building world, please wait.");

	s_time(&ts);
	build_world();
	e_time(ts, &te);

	slog(LG_INFO, "world created in %d msec", tv2ms(&te));
}

static void m_pong(sourceinfo_t *si, int parc, char *parv[])
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

void hijack_pong_handler(void)
{
	pcommand_delete("PONG");
	pcommand_add("PONG", m_pong, 1, MSRC_SERVER);
}

int main(int argc, char *argv[])
{
	atheme_bootstrap();
	atheme_init(argv[0], LOGDIR "/dbverify.log");
	atheme_setup();
	module_t *m;
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
