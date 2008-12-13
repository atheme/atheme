#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/logstream", false, _modinit, _moddeinit,
	"$Id: os_logstream.c 8163 2007-04-07 00:28:09Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_logstream(sourceinfo_t *si, int parc, char *parv[]);

command_t os_logstream = { "LOGSTREAM", "Creates and manipulates logstreams.", PRIV_ADMIN, 2, os_cmd_logstream };

list_t *os_cmdtree;

#ifdef NOTYET
typedef struct logstream_token_ {
	const char *name;
	unsigned int mask;
} logstream_token_t;

logstream_token_t logstream_tokens[2] = {
	{"REGISTER", LG_CMD_REGISTER},
	{"DEBUG", LG_DEBUG},
};
#endif

list_t irc_logstreams = { NULL, NULL, 0 };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");

        command_add(&os_logstream, os_cmdtree);
}

void _moddeinit(void)
{
	node_t *n, *tn;

	command_delete(&os_logstream, os_cmdtree);

	LIST_FOREACH_SAFE(n, tn, irc_logstreams.head)
	{
		logfile_t *lf = n->data;

		object_unref(lf);
		node_del(n, &irc_logstreams);
		node_free(n);
	}
}

static void irc_logstream_write(logfile_t *lf, const char *buf)
{
	msg(opersvs.nick, lf->log_path, "%s", buf);
}

static void irc_logstream_delete(void *vdata)
{
        logfile_t *lf = (logfile_t *) vdata;

	logfile_unregister(lf);
	free(lf->log_path);
	free(lf);
}

static logfile_t *irc_logstream_new(const char *channel, unsigned int mask)
{
	logfile_t *lf = scalloc(sizeof(logfile_t), 1);

	object_init(object(lf), channel, irc_logstream_delete);

	lf->log_path = sstrdup(channel);
	lf->log_mask = mask;
	lf->write_func = irc_logstream_write;

	logfile_register(lf);

	return lf;
}

static void os_cmd_logstream(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan = parv[0];
	node_t *n, *tn;
	logfile_t *lf;

	if (!chan)
	{
		command_fail(si, fault_needmoreparams, "No channel given");
		return;
	}

	LIST_FOREACH_SAFE(n, tn, irc_logstreams.head)
	{
		lf = (logfile_t *) n->data;

		if (!strcasecmp(lf->log_path, chan))
		{
			object_unref(lf);
			node_del(n, &irc_logstreams);
			node_free(n);

			if (irccasecmp(config_options.chan, chan))
				part(chan, si->service->nick);

			command_success_nodata(si, "Removed \2%s\2.", chan);

			return;
		}
	}

	join(chan, si->service->nick);
	lf = irc_logstream_new(chan, LG_CMD_ADMIN | LG_DEBUG);
	n = node_create();
	node_add(lf, n, &irc_logstreams);
	command_success_nodata(si, "Added \2%s\2.", chan);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
