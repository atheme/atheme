/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * servtree.c: Services binary tree manipulation. (add_service, 
 *    del_service, et al.)
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
#include "pmodule.h"

mowgli_patricia_t *services_name;
mowgli_patricia_t *services_nick;
mowgli_heap_t *service_heap;

static void servtree_update(void *dummy);

static void dummy_handler(sourceinfo_t *si, int parc, char **parv)
{
}

static void service_default_handler(sourceinfo_t *si, int parc, char *parv[])
{
	char *cmd;
        char *text;
	char orig[BUFSIZE];

	/* this should never happen */
	if (parv[0][0] == '&')
	{
		slog(LG_ERROR, "services(): got parv with local channel: %s", parv[0]);
		return;
	}

	/* make a copy of the original for debugging */
	strlcpy(orig, parv[parc - 1], BUFSIZE);

	/* lets go through this to get the command */
	cmd = strtok(parv[parc - 1], " ");
	text = strtok(NULL, "");

	if (!cmd)
		return;
	if (*cmd == '\001')
	{
		handle_ctcp_common(si, cmd, text);
		return;
	}

	/* take the command through the hash table */
	command_exec_split(si->service, si, cmd, text, si->service->commands);
}

void servtree_init(void)
{
	service_heap = mowgli_heap_create(sizeof(service_t), 12, BH_NOW);
	services_name = mowgli_patricia_create(strcasecanon);
	services_nick = mowgli_patricia_create(strcasecanon);

	if (!service_heap)
	{
		slog(LG_INFO, "servtree_init(): Block allocator failed.");
		exit(EXIT_FAILURE);
	}

        hook_add_event("config_ready");
	hook_add_config_ready(servtree_update);
}

static void me_me_init(void)
{
	if (me.numeric)
		init_uid();
	me.me = server_add(me.name, 0, NULL, me.numeric ? me.numeric : NULL, me.desc);
}

static void free_alias_string(const char *key, void *data, void *privdata)
{
	free(data);
}

static void create_unique_service_nick(char *dest, size_t len)
{
	unsigned int i = arc4random();

	do
		snprintf(dest, len, "ath%06x", (i++) & 0xFFFFFF);
	while (service_find_nick(dest) || user_find_named(dest));
	return;
}

static int conf_service_nick(config_entry_t *ce)
{
	service_t *sptr;
	char newnick[30 + 1];

	if (!ce->ce_vardata)
		return -1;

	sptr = service_find(ce->ce_prevlevel->ce_varname);
	if (!sptr)
		return -1;

	mowgli_patricia_delete(services_nick, sptr->nick);
	free(sptr->nick);
	if (service_find_nick(ce->ce_vardata))
	{
		create_unique_service_nick(newnick, sizeof newnick);
		slog(LG_INFO, "conf_service_nick(): using nick %s for service %s due to duplication",
				newnick, sptr->internal_name);
		sptr->nick = sstrdup(newnick);
	}
	else
		sptr->nick = sstrdup(ce->ce_vardata);
	mowgli_patricia_add(services_nick, sptr->nick, sptr);

	return 0;
}

static int conf_service_user(config_entry_t *ce)
{
	service_t *sptr;

	if (!ce->ce_vardata)
		return -1;

	sptr = service_find(ce->ce_prevlevel->ce_varname);
	if (!sptr)
		return -1;

	free(sptr->user);
	sptr->user = sstrdup(ce->ce_vardata);

	return 0;
}

static int conf_service_host(config_entry_t *ce)
{
	service_t *sptr;

	if (!ce->ce_vardata)
		return -1;

	if (!is_valid_host(ce->ce_vardata))
	{
		conf_report_warning(ce, "invalid hostname: %s", ce->ce_vardata);
		return -1;
	}

	sptr = service_find(ce->ce_prevlevel->ce_varname);
	if (!sptr)
		return -1;

	free(sptr->host);
	sptr->host = sstrdup(ce->ce_vardata);

	return 0;
}

static int conf_service_real(config_entry_t *ce)
{
	service_t *sptr;

	if (!ce->ce_vardata)
		return -1;

	sptr = service_find(ce->ce_prevlevel->ce_varname);
	if (!sptr)
		return -1;

	free(sptr->real);
	sptr->real = sstrdup(ce->ce_vardata);

	return 0;
}

static int conf_service_aliases(config_entry_t *ce)
{
	service_t *sptr;
	config_entry_t *subce;

	sptr = service_find(ce->ce_prevlevel->ce_varname);
	if (!sptr)
		return -1;

	if (sptr->aliases)
		mowgli_patricia_destroy(sptr->aliases, free_alias_string, NULL);
	sptr->aliases = NULL;
	if (!ce->ce_entries)
		return 0;
	sptr->aliases = mowgli_patricia_create(strcasecanon);
	for (subce = ce->ce_entries; subce != NULL; subce = subce->ce_next)
	{
		if (subce->ce_vardata == NULL || subce->ce_entries != NULL)
		{
			conf_report_warning(subce, "Invalid alias entry");
			continue;
		}
		mowgli_patricia_add(sptr->aliases, subce->ce_varname,
				sstrdup(subce->ce_vardata));
	}

	return 0;
}

static int conf_service(config_entry_t *ce)
{
	service_t *sptr;

	sptr = service_find(ce->ce_varname);
	if (!sptr)
		return -1;

	subblock_handler(ce, sptr->conf_table);
	return 0;
}

service_t *service_add(const char *name, void (*handler)(sourceinfo_t *si, int parc, char *parv[]), mowgli_list_t *conf_table)
{
	service_t *sptr;
	struct ConfTable *subblock;
	const char *nick;
	char newnick[30 + 1];

	if (name == NULL)
	{
		slog(LG_INFO, "service_add(): Bad error! We were given a NULL pointer for service name!");
		return NULL;
	}

	if (service_find(name))
	{
		slog(LG_INFO, "service_add(): Service `%s' already exists.", name);
		return NULL;
	}

	sptr = mowgli_heap_alloc(service_heap);

	sptr->internal_name = sstrdup(name);
	/* default these, to reasonably safe values */
	nick = strchr(name, ':');
	if (nick != NULL)
		nick++;
	else
		nick = name;
	strlcpy(newnick, nick, sizeof newnick);
	if (service_find_nick(newnick))
	{
		create_unique_service_nick(newnick, sizeof newnick);
		slog(LG_INFO, "service_add(): using nick %s for service %s due to duplication",
				newnick, name);
	}
	sptr->nick = sstrdup(newnick);
	sptr->user = sstrndup(nick, 10);
	sptr->host = sstrdup("services.int");
	sptr->real = sstrndup(name, 50);
	sptr->disp = sstrdup(sptr->nick);

	if (handler != NULL)
		sptr->handler = handler;
	else
		sptr->handler = service_default_handler;

	sptr->notice_handler = dummy_handler;
	sptr->aliases = NULL;
	sptr->chanmsg = false;
	sptr->conf_table = conf_table;

	sptr->me = NULL;

	mowgli_patricia_add(services_name, sptr->internal_name, sptr);
	mowgli_patricia_add(services_nick, sptr->nick, sptr);

	sptr->commands = mowgli_patricia_create(strcasecanon);

	if (sptr->conf_table != NULL)
	{
		subblock = find_top_conf(name);
		if (subblock == NULL)
			add_top_conf(sptr->internal_name, conf_service);
		add_conf_item("NICK", sptr->conf_table, conf_service_nick);
		add_conf_item("USER", sptr->conf_table, conf_service_user);
		add_conf_item("HOST", sptr->conf_table, conf_service_host);
		add_conf_item("REAL", sptr->conf_table, conf_service_real);
		add_conf_item("ALIASES", sptr->conf_table, conf_service_aliases);
	}

	return sptr;
}

void service_delete(service_t *sptr)
{
	struct ConfTable *subblock;

	mowgli_patricia_delete(services_name, sptr->internal_name);
	mowgli_patricia_delete(services_nick, sptr->nick);

	if (sptr->conf_table != NULL)
	{
		del_conf_item("ALIASES", sptr->conf_table);
		del_conf_item("REAL", sptr->conf_table);
		del_conf_item("HOST", sptr->conf_table);
		del_conf_item("USER", sptr->conf_table);
		del_conf_item("NICK", sptr->conf_table);
		subblock = find_top_conf(sptr->internal_name);
		if (subblock != NULL && conftable_get_conf_handler(subblock) == conf_service)
			del_top_conf(sptr->internal_name);
	}

	if (sptr->me != NULL)
	{
		quit_sts(sptr->me, "Service unloaded.");
		user_delete(sptr->me, "Service unloaded.");
		sptr->me = NULL;
	}
	sptr->handler = NULL;
	if (sptr->aliases)
		mowgli_patricia_destroy(sptr->aliases, free_alias_string, NULL);
	if (sptr->commands)
		mowgli_patricia_destroy(sptr->commands, NULL, NULL);
	free(sptr->disp);	/* service_name() does a malloc() */
	free(sptr->internal_name);
	free(sptr->nick);
	free(sptr->user);
	free(sptr->host);
	free(sptr->real);

	mowgli_heap_free(service_heap, sptr);
}

service_t *service_add_static(const char *name, const char *user, const char *host, const char *real, void (*handler)(sourceinfo_t *si, int parc, char *parv[]))
{
	service_t *sptr;
	char internal_name[NICKLEN + 10];

	snprintf(internal_name, sizeof internal_name, "static:%s", name);
	sptr = service_add(internal_name, handler, NULL);

	free(sptr->user);
	free(sptr->host);
	free(sptr->real);
	sptr->user = sstrndup(user, 10);
	sptr->host = sstrdup(host);
	sptr->real = sstrdup(real);

	servtree_update(NULL);

	return sptr;
}

service_t *service_find(const char *name)
{
	return mowgli_patricia_retrieve(services_name, name);
}

service_t *service_find_nick(const char *nick)
{
	return mowgli_patricia_retrieve(services_nick, nick);
}

static void servtree_update(void *dummy)
{
	service_t *sptr;
	mowgli_patricia_iteration_state_t state;
	user_t *u;

	if (me.me == NULL)
		me_me_init();
	if (ircd->uses_uid && !me.numeric)
	{
		slog(LG_ERROR, "servtree_update(): ircd requires numeric, but none was specified in the configuration file");
		exit(EXIT_FAILURE);
	}
	
	MOWGLI_PATRICIA_FOREACH(sptr, &state, services_name)
	{
		free(sptr->disp);
		sptr->disp = service_name(sptr->nick);
		if (sptr->me != NULL)
		{
			if (!strcmp(sptr->nick, sptr->me->nick) &&
					!strcmp(sptr->user, sptr->me->user) &&
					!strcmp(sptr->host, sptr->me->host) &&
					!strcmp(sptr->real, sptr->me->gecos))
				continue;
			if (me.connected)
				quit_sts(sptr->me, "Updating information");
			u = user_find_named(sptr->nick);
			if (u != NULL && u != sptr->me)
				kill_user(NULL, u, "Nick taken by service");
			user_changenick(sptr->me, sptr->nick, CURRTIME);
			strlcpy(sptr->me->user, sptr->user, sizeof sptr->me->user);
			strlcpy(sptr->me->host, sptr->host, sizeof sptr->me->host);
			strlcpy(sptr->me->gecos, sptr->real, sizeof sptr->me->gecos);
			if (me.connected)
				reintroduce_user(sptr->me);
		}
		else
		{
			u = user_find_named(sptr->nick);
			if (u != NULL)
				kill_user(NULL, u, "Nick taken by service");
			sptr->me = user_add(sptr->nick, sptr->user, sptr->host, NULL, NULL, ircd->uses_uid ? uid_get() : NULL, sptr->real, me.me, CURRTIME);
			sptr->me->flags |= UF_IRCOP | UF_INVIS;
			if ((sptr == chansvs.me || sptr->conf_table == NULL) &&
				!chansvs.fantasy)
				sptr->me->flags |= UF_DEAF;

			if (me.connected)
			{
				/*
				 * Possibly send a kill for the service nick now
				 * - do not send a kill if we already sent
				 *   one above
				 * - do not send a kill if we use UID, with
				 *   UID we can always kill the other user
				 *   in nick collisions and killing a nick
				 *   with UID is likely to cause the desyncs
				 *   UID is meant to avoid
				 */
				if (u == NULL && !ircd->uses_uid)
					kill_id_sts(NULL, sptr->nick, "Attempt to use service nick");
				introduce_nick(sptr->me);
				/* if the snoop channel already exists, join it now */
				if (config_options.chan != NULL && channel_find(config_options.chan) != NULL)
					join(config_options.chan, sptr->nick);
				hook_call_service_introduce(sptr);
			}
		}
	}
}

char *service_name(char *name)
{
	char *str;

	if (config_options.secure)
	{
		str = smalloc(strlen(name) + 1 + strlen(me.name) + 1);
		sprintf(str, "%s@%s", name, me.name);
	}
	else
		str = sstrdup(name);

	return str;
}

void service_set_chanmsg(service_t *service, bool chanmsg)
{
	return_if_fail(service != NULL);

	service->chanmsg = chanmsg;
}

const char *service_resolve_alias(service_t *sptr, const char *context, const char *cmd)
{
	char fullname[256];
	char *alias;

	if (sptr->aliases == NULL)
		return cmd;
	fullname[0] = '\0';
	if (context != NULL)
	{
		strlcpy(fullname, context, sizeof fullname);
		strlcat(fullname, " ", sizeof fullname);
	}
	strlcat(fullname, cmd, sizeof fullname);
	alias = mowgli_patricia_retrieve(sptr->aliases, fullname);
	return alias != NULL ? alias : cmd;
}

void service_bind_command(service_t *sptr, command_t *cmd)
{
	return_if_fail(sptr != NULL);
	return_if_fail(cmd != NULL);

	command_add(cmd, sptr->commands);
}

void service_unbind_command(service_t *sptr, command_t *cmd)
{
	return_if_fail(sptr != NULL);
	return_if_fail(cmd != NULL);

	command_delete(cmd, sptr->commands);
}

void service_named_bind_command(const char *svs, command_t *cmd)
{
	service_t *sptr = NULL;

	return_if_fail(svs != NULL);
	return_if_fail(cmd != NULL);

	sptr = service_find(svs);
	service_bind_command(sptr, cmd);
}

void service_named_unbind_command(const char *svs, command_t *cmd)
{
	service_t *sptr = NULL;

	return_if_fail(svs != NULL);
	return_if_fail(cmd != NULL);

	sptr = service_find(svs);
	service_unbind_command(sptr, cmd);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
