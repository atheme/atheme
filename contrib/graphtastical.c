/*
 * Copyright (c) 2007 William Pitcock
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Creates a .dot file for use with neato which displays
 * user->channel relationships.
 *
 * == How to generate the graphs and How It Works ==
 * Graphtastical creates a .dot file for graphviz's neato
 * filter to use. The DOT language describes a graph's
 * structure to graphviz in an opaque way.
 *
 * Because Graphviz nodes use unique identifiers for
 * interconnection, the channels.dot file contains also
 * information about social networks.
 *
 * Eventually Graphtastical will dump other graph datafiles
 * too.
 *
 * To make a file from the data dumped by Graphtastical,
 * the following commands will do:
 *
 * $ cat channels.dot | neato -Tgif -o map-channels.gif
 * $ cat channels.dot | neato -Tsvg -o map-channels.svg
 *
 * Some maps (for larger networks) are going to be large,
 * so you may want to provide links to both the GIF and
 * SVG files as some people may only be able to make use of
 * one or the other. Why that is, I'm not sure, and I'm not
 * covering it here.
 *
 * == Privacy concerns ==
 * If you are running Graphtastical on a network that has
 * privacy concerns; you probably shouldn't.
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"contrib/graphtastical", TRUE, _modinit, NULL,
	"$Id: flatfile.c 8329 2007-05-27 13:31:59Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

/* write channels.dot */
static void write_channels_dot_file(void *arg)
{
	myuser_t *mu;
	mychan_t *mc;
	chanacs_t *ca;
	kline_t *k;
	svsignore_t *svsignore;
	soper_t *soper;
	node_t *n, *tn, *tn2;
	FILE *f;
	int errno1, was_errored = 0;
	dictionary_iteration_state_t state;
	int root = 1;
	mychan_t *pmc;

	errno = 0;

	/* write to a temporary file first */
	if (!(f = fopen(DATADIR "/channels.dot.new", "w")))
	{
		errno1 = errno;
		slog(LG_ERROR, "graphtastical: cannot create channels.dot.new: %s", strerror(errno1));
		return;
	}

	fprintf(f, "graph channels-graph {\n");
	fprintf(f, "edge [color=blue w=5.0 len=1.5 fontsize=10]\n");
	fprintf(f, "node [fontsize=10]\n");

	slog(LG_DEBUG, "graphtastical: dumping mychans");

	DICTIONARY_FOREACH(mc, &state, mclist)
	{
		fprintf(f, "\"%s\"", mc->name);

		if (!root)
			fprintf(f, "-- \"%s\"", pmc->name);

		pmc = mc;

		fprintf(f, "[fontsize=10]\n");

		LIST_FOREACH(tn, mc->chanacs.head)
		{
			ca = (chanacs_t *)tn->data;

			if (ca->level & CA_AKICK)
				continue;

			fprintf(f, "\"%s\" -- \"%s\" [fontsize=10]\n", ca->myuser ? ca->myuser->name : ca->host, mc->name);
		}
	}

	fprintf(f, "}\n");

	was_errored = ferror(f);
	was_errored |= fclose(f);
	if (was_errored)
	{
		errno1 = errno;
		slog(LG_ERROR, "db_save(): cannot write to channels.dot.new: %s", strerror(errno1));
		return;
	}

	/* now, replace the old database with the new one, using an atomic rename */
	unlink(DATADIR "/channels.dot" );
	
	if ((rename(DATADIR "/channels.dot.new", DATADIR "/channels.dot")) < 0)
	{
		errno1 = errno;
		slog(LG_ERROR, "graphtastical: cannot rename channels.dot.new to channels.dot: %s", strerror(errno1));
		return;
	}
}

void _modinit(module_t *m)
{
	write_channels_dot_file(NULL);

	event_add("write_channels_dot_file", write_channels_dot_file, NULL, 60);
}

void _moddeinit(void)
{
	event_delete(write_channels_dot_file, NULL);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
