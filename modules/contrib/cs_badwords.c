/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"contrib/cs_badwords", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.net>"
);

static void on_channel_message(hook_cmessage_data_t *data);
static void cs_cmd_badwords(sourceinfo_t *si, int parc, char *parv[]);
static void cs_set_cmd_blockbadwords(sourceinfo_t *si, int parc, char *parv[]);

static void write_badword_db(database_handle_t *db);
static void db_h_bw(database_handle_t *db, const char *type);

command_t cs_badwords = { "BADWORDS", N_("Manage the list of channel bad words."), AC_AUTHENTICATED, 4, cs_cmd_badwords, { .path = "contrib/badwords" } };
command_t cs_set_blockbadwords = { "BLOCKBADWORDS", N_("Set whether users can say badwords in channel or not."), AC_NONE, 2, cs_set_cmd_blockbadwords, { .path = "contrib/set_blockbadwords" } };

struct badword_ {
	char *badword;
	time_t add_ts;
	char *creator;
	char *channel;
	char *action;
	mowgli_node_t node;
};

typedef struct badword_ badword_t;

mowgli_patricia_t **cs_set_cmdtree;

void _modinit(module_t *m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, cs_set_cmdtree, "chanserv/set_core", "cs_set_cmdtree");

	if (!module_find_published("backend/opensex"))
	{
		slog(LG_INFO, "Module %s requires use of the OpenSEX database backend, refusing to load.", m->header->name);
		m->mflags = MODTYPE_FAIL;
		return;
	}

	hook_add_event("channel_message");
	hook_add_channel_message(on_channel_message);

	hook_add_db_write(write_badword_db);

	db_register_type_handler("BW", db_h_bw);

	service_named_bind_command("chanserv", &cs_badwords);
	command_add(&cs_set_blockbadwords, *cs_set_cmdtree);
}

void _moddeinit(module_unload_intent_t intent)
{
	hook_del_channel_message(on_channel_message);
	hook_del_db_write(write_badword_db);

	db_unregister_type_handler("BW");

	service_named_unbind_command("chanserv", &cs_badwords);
	command_delete(&cs_set_blockbadwords, *cs_set_cmdtree);
}

static void write_badword_db(database_handle_t *db)
{
	mowgli_node_t *n;
	mychan_t *mc;
	mowgli_patricia_iteration_state_t state;
	mowgli_list_t *l;

	MOWGLI_PATRICIA_FOREACH(mc, &state, mclist)
	{
		l = privatedata_get(mc, "badword:list");

		if (l == NULL)
			return;

		MOWGLI_ITER_FOREACH(n, l->head)
		{
			badword_t *bw = n->data;

			db_start_row(db, "BW");
			db_write_word(db, bw->badword);
			db_write_time(db, bw->add_ts);
			db_write_word(db, bw->creator);
			db_write_word(db, bw->channel);
			db_write_word(db, bw->action);
			db_commit_row(db);
		}
	}
}

static void db_h_bw(database_handle_t *db, const char *type)
{
	mychan_t *mc;
	mowgli_patricia_iteration_state_t state;
	mowgli_list_t *l;

	const char *badword = db_sread_word(db);
	time_t add_ts = db_sread_time(db);
	const char *creator = db_sread_word(db);
	const char *channel = db_sread_word(db);
	const char *action = db_sread_word(db);

	MOWGLI_PATRICIA_FOREACH(mc, &state, mclist)
	{
		if (irccasecmp(mc->name, channel))
			continue;

		l = privatedata_get(mc, "badword:list");

		if (l == NULL)
			l = mowgli_list_create();

		badword_t *bw = smalloc(sizeof(badword_t));
		bw->badword = sstrdup(badword);
		bw->add_ts = add_ts;;
		bw->creator = sstrdup(creator);
		bw->channel = sstrdup(channel);
		bw->action = sstrdup(action);
		mowgli_node_add(bw, &bw->node, l);
		privatedata_set(mc, "badword:list", l);
	}
}

static void on_channel_message(hook_cmessage_data_t *data)
{
	badword_t *bw;
	mowgli_node_t *n;
	mowgli_list_t *l;
	
	mychan_t *mc = mychan_find(data->c->name);

	if (metadata_find(mc, "blockbadwords") == NULL)
		return;

	l = privatedata_get(mc, "badword:list");

	if (l == NULL)
		return;

	char *kickstring = "Foul language is prohibited here.";

	if (data != NULL && data->msg != NULL)
	{
		MOWGLI_ITER_FOREACH(n, l->head)
		{
			bw = n->data;

			if (!match(bw->badword, data->msg))
			{

				if (!strcasecmp("KICKBAN", bw->action))
				{
					char hostbuf[BUFSIZE];

					hostbuf[0] = '\0';

					strlcat(hostbuf, "*!*@", BUFSIZE);
					strlcat(hostbuf, data->u->vhost, BUFSIZE);

					modestack_mode_param(chansvs.nick, data->c, MTYPE_ADD, 'b', hostbuf);
					chanban_add(data->c, hostbuf, 'b');
					kick(chansvs.me->me, data->c, data->u, kickstring);
					return;
				}
				else if (!strcasecmp("KICK", bw->action))
				{
					kick(chansvs.me->me, data->c, data->u, kickstring);
					return;
				}
				else if (!strcasecmp("QUIET", bw->action))
				{
					char hostbuf[BUFSIZE];

					hostbuf[0] = '\0';

					strlcat(hostbuf, "*!*@", BUFSIZE);
					strlcat(hostbuf, data->u->vhost, BUFSIZE);

					modestack_mode_param(chansvs.nick, data->c, MTYPE_ADD, 'q', hostbuf);
					chanban_add(data->c, hostbuf, 'q');
					return;
				}
				else if (!strcasecmp("BAN", bw->action))
				{
					char hostbuf[BUFSIZE];

					hostbuf[0] = '\0';

					strlcat(hostbuf, "*!*@", BUFSIZE);
					strlcat(hostbuf, data->u->vhost, BUFSIZE);

					modestack_mode_param(chansvs.nick, data->c, MTYPE_ADD, 'b', hostbuf);
					chanban_add(data->c, hostbuf, 'b');
					return;
				}
			}
		}
	}
}




/* SET BADWORD */
static void cs_cmd_badwords(sourceinfo_t *si, int parc, char *parv[])
{
	char *channel = parv[0];
	char *command = parv[1];
	char *word = parv[2];
	char *action = parv[3];
	mychan_t *mc;
	mowgli_node_t *n, *tn;
	badword_t *bw;
	mowgli_list_t *l;

	if (!channel || !command)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET BADWORDS");
		command_fail(si, fault_needmoreparams, _("Syntax: BADWORDS <#channel> ADD|DEL|LIST [badword] [action]"));
		return;
	}

	if (!(mc = mychan_find(channel)))
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), channel);
		return;
	}

	/* Do this here to avoid duplicating this line */
	l = privatedata_get(mc, "badword:list");

	if (!strcasecmp("ADD", command))
	{

		if (!word || !action)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "BADWORDS");
			command_fail(si, fault_needmoreparams, _("Syntax: BADWORDS <#channel> ADD <badword> <action>"));
			return;
		}

		if (!chanacs_source_has_flag(mc, si, CA_SET))
		{
			command_fail(si, fault_noprivs, _("You are not authorized to perform this command."));
			return;
		}

		if(!strcasecmp("KICK", action) || !strcasecmp("KICKBAN", action) || !strcasecmp("BAN", action) || (!strcasecmp("QUIET", action) && ircd != NULL && strchr(ircd->ban_like_modes, 'q')))
		{
			if (l != NULL)
			{
				MOWGLI_ITER_FOREACH(n, l->head)
				{
					bw = n->data;

					if (!irccasecmp(bw->badword, word))
					{	
						command_success_nodata(si, _("\2%s\2 has already been entered into the bad word list."), word);
						return;
					}
				}
			}

			if (l == NULL)
				l = mowgli_list_create();

			bw = smalloc(sizeof(badword_t));
			bw->add_ts = CURRTIME;;
			bw->creator = sstrdup(get_source_name(si));
			bw->channel = sstrdup(mc->name);
			bw->badword = sstrdup(word);
			bw->action = sstrdup(action);
			mowgli_node_add(bw, &bw->node, l);
			privatedata_set(mc, "badword:list", l);

			command_success_nodata(si, _("You have added \2%s\2 as a bad word."), word);
			logcommand(si, CMDLOG_SET, "BADWORDS:ADD: \2%s\2 \2%s\2 \2%s\2", channel, word, action);
		}
		else
		{
			command_fail(si, fault_badparams, _("Invalid action given."));
			return;
		}
	}
	else if (!strcasecmp("DEL", command))
	{

		if (!word)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "BADWORDS");
			command_fail(si, fault_needmoreparams, _("Syntax: BADWORDS <#channel> DEL <badword>"));
			return;
		}

		if (!chanacs_source_has_flag(mc, si, CA_SET))
		{
			command_fail(si, fault_noprivs, _("You are not authorized to perform this command."));
			return;
		}

		if (l == NULL)
		{
			command_fail(si, fault_nosuch_target, _("There are no badwords set in this channel."));
			return;
		}
	
		MOWGLI_ITER_FOREACH_SAFE(n, tn, l->head)
		{
			bw = n->data;

			if (!irccasecmp(bw->badword, word))
			{
				logcommand(si, CMDLOG_SET, "BADWORDS:DEL: \2%s\2 \2%s\2", mc->name, bw->badword);
				command_success_nodata(si, _("Bad word \2%s\2 has been deleted."), bw->badword);

				mowgli_node_delete(&bw->node, l);

				free(bw->creator);
				free(bw->channel);
				free(bw->badword);
				free(bw->action);
				free(bw);

				privatedata_set(mc, "badword:list", l);

				return;
				}
		}
		command_success_nodata(si, _("Word \2%s\2 not found in bad word database."), word);
	}
	else if (!strcasecmp("LIST", command))
	{
		char buf[BUFSIZE];
		struct tm tm;
		
		if (!chanacs_source_has_flag(mc, si, CA_ACLVIEW))
		{
			command_fail(si, fault_noprivs, _("You are not authorized to perform this command."));
			return;
		}
		
		if (l == NULL)
		{
			command_fail(si, fault_nosuch_target, _("There are no badwords set in this channel."));
			return;
		}

		MOWGLI_ITER_FOREACH(n, l->head)
		{
			bw = n->data;

			tm = *localtime(&bw->add_ts);
			strftime(buf, BUFSIZE, config_options.time_format, &tm);
			command_success_nodata(si, "Word: \2%s\2 Action: \2%s\2 (%s - %s)",
				bw->badword, bw->action, bw->creator, buf);
		}
		command_success_nodata(si, "End of list.");
		logcommand(si, CMDLOG_GET, "BADWORDS:LIST");
	}
	else
	{
		command_fail(si, fault_needmoreparams, STR_INVALID_PARAMS, "BADWORDS");
		command_fail(si, fault_needmoreparams, _("Syntax: BADWORDS <#channel> ADD|DEL|LIST [badword] [action]"));
		return;
	}
}

static void cs_set_cmd_blockbadwords(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), parv[0]);
		return;
	}

	if (!parv[1])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET BLOCKBADWORDS");
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this command."));
		return;
	}

	if (!strcasecmp("ON", parv[1]))
	{
		metadata_t *md = metadata_find(mc, "blockbadwords");

		if (md)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is already set for channel \2%s\2."), "BLOCKBADWORDS", mc->name);
			return;
		}

		metadata_add(mc, "blockbadwords", "on");

		logcommand(si, CMDLOG_SET, "SET:BLOCKBADWORDS:ON: \2%s\2", mc->name);
		command_success_nodata(si, _("The \2%s\2 flag has been set for channel \2%s\2."), "BLOCKBADWORDS", mc->name);
		return;
	}
	else if (!strcasecmp("OFF", parv[1]))
	{
		metadata_t *md = metadata_find(mc, "blockbadwords");

		if (!md)
		{
			command_fail(si, fault_nochange, _("The \2%s\2 flag is not set for channel \2%s\2."), "BLOCKBADWORDS", mc->name);
			return;
		}

		metadata_delete(mc, "blockbadwords");

		logcommand(si, CMDLOG_SET, "SET:BLOCKBADWORDS:OFF: \2%s\2", mc->name);
		command_success_nodata(si, _("The \2%s\2 flag has been removed for channel \2%s\2."), "BLOCKBADWORDS", mc->name);
		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "BLOCKBADWORDS");
		return;
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
