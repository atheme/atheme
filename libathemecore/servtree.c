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

void servtree_update(void *dummy);

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
	mowgli_strlcpy(orig, parv[parc - 1], BUFSIZE);

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
	service_heap = sharedheap_get(sizeof(service_t));
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

static void free_access_string(const char *key, void *data, void *privdata)
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

static int conf_service_nick(mowgli_config_file_entry_t *ce)
{
	service_t *sptr;
	char newnick[NICKLEN + 1];

	if (!ce->vardata)
		return -1;

	sptr = service_find(ce->prevlevel->varname);
	if (!sptr)
		return -1;

	mowgli_patricia_delete(services_nick, sptr->nick);
	free(sptr->nick);

	if (service_find_nick(ce->vardata))
	{
		create_unique_service_nick(newnick, sizeof newnick);
		slog(LG_INFO, "conf_service_nick(): using nick %s for service %s due to duplication",
				newnick, sptr->internal_name);
		sptr->nick = sstrdup(newnick);
	}
	else
		sptr->nick = sstrdup(ce->vardata);
	mowgli_patricia_add(services_nick, sptr->nick, sptr);

	return 0;
}

static int conf_service_user(mowgli_config_file_entry_t *ce)
{
	service_t *sptr;

	if (!ce->vardata)
		return -1;

	sptr = service_find(ce->prevlevel->varname);
	if (!sptr)
		return -1;

	free(sptr->user);
	sptr->user = sstrndup(ce->vardata, 10);

	return 0;
}

static int conf_service_host(mowgli_config_file_entry_t *ce)
{
	service_t *sptr;

	if (!ce->vardata)
		return -1;

	if (!is_valid_host(ce->vardata))
	{
		conf_report_warning(ce, "invalid hostname: %s", ce->vardata);
		return -1;
	}

	sptr = service_find(ce->prevlevel->varname);
	if (!sptr)
		return -1;

	free(sptr->host);
	sptr->host = sstrdup(ce->vardata);

	return 0;
}

static int conf_service_real(mowgli_config_file_entry_t *ce)
{
	service_t *sptr;

	if (!ce->vardata)
		return -1;

	sptr = service_find(ce->prevlevel->varname);
	if (!sptr)
		return -1;

	free(sptr->real);
	sptr->real = sstrdup(ce->vardata);

	return 0;
}

static int conf_service_aliases(mowgli_config_file_entry_t *ce)
{
	service_t *sptr;
	mowgli_config_file_entry_t *subce;

	sptr = service_find(ce->prevlevel->varname);
	if (!sptr)
		return -1;

	if (sptr->aliases)
		mowgli_patricia_destroy(sptr->aliases, free_alias_string, NULL);

	sptr->aliases = NULL;
	if (!ce->entries)
		return 0;

	sptr->aliases = mowgli_patricia_create(strcasecanon);

	MOWGLI_ITER_FOREACH(subce, ce->entries)
	{
		if (subce->vardata == NULL || subce->entries != NULL)
		{
			conf_report_warning(subce, "Invalid alias entry");
			continue;
		}

		mowgli_patricia_add(sptr->aliases, subce->varname,
				sstrdup(subce->vardata));
	}

	return 0;
}

static int conf_service_access(mowgli_config_file_entry_t *ce)
{
	service_t *sptr;
	mowgli_config_file_entry_t *subce;

	sptr = service_find(ce->prevlevel->varname);
	if (!sptr)
		return -1;

	if (sptr->access)
		mowgli_patricia_destroy(sptr->access, free_access_string, NULL);

	sptr->access = NULL;
	if (!ce->entries)
		return 0;

	sptr->access = mowgli_patricia_create(strcasecanon);

	MOWGLI_ITER_FOREACH(subce, ce->entries)
	{
		if (subce->vardata == NULL || subce->entries != NULL)
		{
			conf_report_warning(subce, "Invalid access entry");
			continue;
		}

		mowgli_patricia_add(sptr->access, subce->varname,
				sstrdup(subce->vardata));
	}

	return 0;
}

static int conf_service(mowgli_config_file_entry_t *ce)
{
	service_t *sptr;

	sptr = service_find(ce->varname);
	if (!sptr)
		return -1;

	subblock_handler(ce, &sptr->conf_table);
	return 0;
}

service_t *service_add(const char *name, void (*handler)(sourceinfo_t *si, int parc, char *parv[]))
{
	service_t *sptr;
	struct ConfTable *subblock;
	const char *nick;
	char newnick[NICKLEN];

	return_val_if_fail(name != NULL, NULL);
	return_val_if_fail(service_find(name) == NULL, NULL);

	sptr = mowgli_heap_alloc(service_heap);

	sptr->internal_name = sstrdup(name);
	/* default these, to reasonably safe values */
	nick = strchr(name, ':');
	if (nick != NULL)
		nick++;
	else
		nick = name;
	mowgli_strlcpy(newnick, nick, sizeof newnick);
	if (!is_valid_nick(newnick) || service_find_nick(newnick))
	{
		create_unique_service_nick(newnick, sizeof newnick);
		slog(LG_INFO, "service_add(): using nick %s for service %s due to duplicate or invalid nickname",
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
	sptr->access = NULL;
	sptr->chanmsg = false;

	sptr->me = NULL;

	mowgli_patricia_add(services_name, sptr->internal_name, sptr);
	mowgli_patricia_add(services_nick, sptr->nick, sptr);

	sptr->commands = mowgli_patricia_create(strcasecanon);

	subblock = find_top_conf(name);
	if (subblock == NULL)
		add_top_conf(sptr->internal_name, conf_service);
	add_conf_item("NICK", &sptr->conf_table, conf_service_nick);
	add_conf_item("USER", &sptr->conf_table, conf_service_user);
	add_conf_item("HOST", &sptr->conf_table, conf_service_host);
	add_conf_item("REAL", &sptr->conf_table, conf_service_real);
	add_conf_item("ALIASES", &sptr->conf_table, conf_service_aliases);
	add_conf_item("ACCESS", &sptr->conf_table, conf_service_access);

	return sptr;
}

void service_delete(service_t *sptr)
{
	struct ConfTable *subblock;

	mowgli_patricia_delete(services_name, sptr->internal_name);
	mowgli_patricia_delete(services_nick, sptr->nick);

	del_conf_item("ACCESS", &sptr->conf_table);
	del_conf_item("ALIASES", &sptr->conf_table);
	del_conf_item("REAL", &sptr->conf_table);
	del_conf_item("HOST", &sptr->conf_table);
	del_conf_item("USER", &sptr->conf_table);
	del_conf_item("NICK", &sptr->conf_table);
	subblock = find_top_conf(sptr->internal_name);
	if (subblock != NULL && conftable_get_conf_handler(subblock) == conf_service)
		del_top_conf(sptr->internal_name);

	if (sptr->me != NULL)
	{
		quit_sts(sptr->me, "Service unloaded.");
		user_delete(sptr->me, "Service unloaded.");
		sptr->me = NULL;
	}
	sptr->handler = NULL;
	if (sptr->access)
		mowgli_patricia_destroy(sptr->access, free_access_string, NULL);
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
	sptr = service_add(internal_name, handler);

	free(sptr->user);
	free(sptr->host);
	free(sptr->real);
	sptr->user = sstrndup(user, 10);
	sptr->host = sstrdup(host);
	sptr->real = sstrdup(real);
	sptr->botonly = true;

	servtree_update(NULL);

	return sptr;
}

service_t *service_find_any(void)
{
	service_t *sptr;
	mowgli_patricia_iteration_state_t state;

	MOWGLI_PATRICIA_FOREACH(sptr, &state, services_name)
		return sptr;
	return NULL;
}

service_t *service_find(const char *name)
{
	return mowgli_patricia_retrieve(services_name, name);
}

service_t *service_find_nick(const char *nick)
{
	return mowgli_patricia_retrieve(services_nick, nick);
}

void servtree_update(void *dummy)
{
	service_t *sptr;
	mowgli_patricia_iteration_state_t state;
	user_t *u;

	if (offline_mode)
		return;

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
			strshare_unref(sptr->me->user);
			sptr->me->user = strshare_get(sptr->user);
			strshare_unref(sptr->me->host);
			sptr->me->host = strshare_get(sptr->host);
			strshare_unref(sptr->me->chost);
			sptr->me->chost = strshare_ref(sptr->me->host);
			strshare_unref(sptr->me->vhost);
			sptr->me->vhost = strshare_ref(sptr->me->host);
			strshare_unref(sptr->me->gecos);
			sptr->me->gecos = strshare_get(sptr->real);
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
			if ((sptr == chansvs.me) && !chansvs.fantasy)
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
		mowgli_strlcpy(fullname, context, sizeof fullname);
		mowgli_strlcat(fullname, " ", sizeof fullname);
	}
	mowgli_strlcat(fullname, cmd, sizeof fullname);
	alias = mowgli_patricia_retrieve(sptr->aliases, fullname);
	return alias != NULL ? alias : cmd;
}

const char *service_set_access(service_t *sptr, const char *cmd, const char *oldaccess)
{
	char *newaccess;

	if (sptr->access == NULL)
		return oldaccess;

	newaccess = mowgli_patricia_retrieve(sptr->access, cmd);
	return newaccess != NULL ? newaccess : oldaccess;
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
