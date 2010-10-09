/*
 * Copyright (c) 2008 Jilles Tjoelker
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Dump/restore channel modes
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/savechanmodes", false, _modinit, _moddeinit,
	"$Revision: 7785 $",
	"Jilles Tjoelker <jilles -at- stack.nl>"
);

static void os_cmd_savechanmodes(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_loadchanmodes(sourceinfo_t *si, int parc, char *parv[]);

command_t os_savechanmodes = { "SAVECHANMODES", "Dumps channel modes to a file.",
		  	   PRIV_ADMIN, 1, os_cmd_savechanmodes, { .path = "contrib/savechanmodes" } };
command_t os_loadchanmodes = { "LOADCHANMODES", "Restores channel modes from a file.",
		  	   PRIV_ADMIN, 1, os_cmd_loadchanmodes, { .path = "contrib/loadchanmodes" } };

void _modinit(module_t *m)
{
	service_named_bind_command("operserv", &os_savechanmodes);
	service_named_bind_command("operserv", &os_loadchanmodes);
}

void _moddeinit(void)
{
	service_named_unbind_command("operserv", &os_savechanmodes);
	service_named_unbind_command("operserv", &os_loadchanmodes);
}

static void os_cmd_savechanmodes(sourceinfo_t *si, int parc, char *parv[])
{
	FILE *out;
	mowgli_patricia_iteration_state_t state;
	channel_t *c;
	mowgli_node_t *n;
	chanban_t *cb;

	if (!(out = fopen(DATADIR "/chanmodes.txt", "w")))
	{
		command_fail(si, fault_nosuch_source, "Cannot open %s: %s",
				DATADIR "/chanmodes.txt", strerror(errno));
		return;
	}

	logcommand(si, CMDLOG_ADMIN, "SAVECHANMODES");
	wallops("\2%s\2 is dumping channel modes", get_oper_name(si));

	MOWGLI_PATRICIA_FOREACH(c, &state, chanlist)
	{
		fprintf(out, "chan %s %s\n", c->name, channel_modes(c, true));
		if (c->topic)
			fprintf(out, "topic %s %lu %s\n", c->topic_setter,
					(unsigned long)c->topicts,
					c->topic);
		MOWGLI_ITER_FOREACH(n, c->bans.head)
		{
			cb = n->data;
			fprintf(out, "ban %c %s\n", cb->type, cb->mask);
		}
	}

	fclose(out);

	command_success_nodata(si, "Channel modes saved to %s.",
			DATADIR "/chanmodes.txt");
}

static channel_t *restore_channel(char *name, char *modes)
{
	channel_t *c;
	int modeparc;
	char *modeparv[256];
	service_t *svs;

	svs = service_find("operserv");
	if (svs == NULL)
		svs = chansvs.me;

	join(name, chansvs.nick);
	c = channel_find(name);
	if (c != NULL)
	{
		modeparc = sjtoken(modes, ' ', modeparv);
		channel_mode(svs->me, c, modeparc, modeparv);
	}
	return c;
}

static void os_cmd_loadchanmodes(sourceinfo_t *si, int parc, char *parv[])
{
	FILE *in;
	char *item, buf[2048];
	char *name, *modes, *setter, *tsstr, *topic, *type, *mask;
	time_t ts, prevtopicts;
	channel_t *c;
	int line;

	if (!(in = fopen(DATADIR "/chanmodes.txt", "r")))
	{
		command_fail(si, fault_nosuch_source, "Cannot open %s: %s",
				DATADIR "/chanmodes.txt", strerror(errno));
		return;
	}

	logcommand(si, CMDLOG_ADMIN, "LOADCHANMODES");
	wallops("\2%s\2 is restoring channel modes", get_oper_name(si));

	line = 0;
	c = NULL;
	while (fgets(buf, sizeof buf, in))
	{
		line++;
		item = strtok(buf, " ");
		strip(item);

		if (item == NULL || *item == '#')
			continue;

		if (!strcmp(item, "chan"))
		{
			name = strtok(NULL, " ");
			modes = strtok(NULL, "\n");

			if (name == NULL || modes == NULL)
				continue;
			c = restore_channel(name, modes);
		}
		else if (!strcmp(item, "topic"))
		{
			if (c == NULL)
				continue;

			setter = strtok(NULL, " ");
			tsstr = strtok(NULL, " ");
			topic = strtok(NULL, "\n");

			if (setter == NULL || tsstr == NULL || topic == NULL)
				continue;
			if (c->topic != NULL)
				continue;
			prevtopicts = c->topicts;
			ts = strtoul(tsstr, NULL, 10);
			handle_topic(c, setter, ts, topic);
			topic_sts(c, chansvs.me->me, setter, ts, prevtopicts, topic);
		}
		else if (!strcmp(item, "ban"))
		{
			if (c == NULL)
				continue;

			type = strtok(NULL, " ");
			mask = strtok(NULL, "\n");

			if (type == NULL || mask == NULL)
				continue;
			modestack_mode_param(chansvs.nick, c, MTYPE_ADD, type[0], mask);
			chanban_add(c, mask, type[0]);
		}
	}

	fclose(in);

	command_success_nodata(si, "Channel modes restored from %s.",
			DATADIR "/chanmodes.txt");
	command_success_nodata(si, "Remember to restart services to make %s leave channels it should not be in.",
			chansvs.nick);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
