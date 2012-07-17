/*
 * Copyright (c) 2012 William Pitcock <nenolod@dereferenced.org>.
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
#include "chanserv.h"

DECLARE_MODULE_V1
(
	"chanserv/moderate", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_activate(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_reject(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_waiting(sourceinfo_t *si, int parc, char *parv[]);
static void can_register(hook_channel_register_check_t *req);

static command_t cs_activate = { "ACTIVATE", N_("Activates a pending registration"), PRIV_CHAN_ADMIN,
				 2, cs_cmd_activate, { .path = "chanserv/activate" } };
static command_t cs_reject   = { "REJECT", N_("Rejects a pending registration"), PRIV_CHAN_ADMIN,
				 2, cs_cmd_reject, { .path = "chanserv/reject" } };
static command_t cs_waiting  = { "WAITING", N_("View pending registrations"), PRIV_CHAN_ADMIN,
				 1, cs_cmd_waiting, { .path = "chanserv/waiting" } };

typedef struct {
	char *name;
	myentity_t *mt;
	time_t ts;
} csreq_t;

static mowgli_patricia_t *csreq_list = NULL;
static char *groupmemo;

/*****************************************************************************/

static csreq_t *csreq_create(const char *name, myentity_t *mt)
{
	csreq_t *cs;

	return_val_if_fail(name != NULL, NULL);
	return_val_if_fail(mt != NULL, NULL);

	cs = smalloc(sizeof(csreq_t));
	cs->name = sstrdup(name);
	cs->mt = mt;
	cs->ts = CURRTIME;

	mowgli_patricia_add(csreq_list, cs->name, cs);

	return cs;
}

static csreq_t *csreq_find(const char *name)
{
	return_val_if_fail(name != NULL, NULL);

	return mowgli_patricia_retrieve(csreq_list, name);
}

static void csreq_destroy(csreq_t *cs)
{
	return_if_fail(cs != NULL);

	mowgli_patricia_delete(csreq_list, cs->name);
	free(cs->name);
	free(cs);
}

static void csreq_demarshal(database_handle_t *db, const char *type)
{
        const char *chan = db_sread_word(db);
        const char *nick = db_sread_word(db);
        time_t req_ts = db_sread_time(db);
	myentity_t *mt;
	csreq_t *cs;

	mt = myentity_find(nick);
	if (mt == NULL)
	{
		slog(LG_INFO, "csreq_demarshal(): couldn't find entity for '%s'", nick);
		return;
	}

	cs = csreq_create(chan, mt);
	cs->ts = req_ts;
}

static void csreq_marshal_set(database_handle_t *db)
{
	mowgli_patricia_iteration_state_t state;
	csreq_t *cs;

	MOWGLI_PATRICIA_FOREACH(cs, &state, csreq_list)
	{
		db_start_row(db, "CSREQ");
		db_write_word(db, cs->name);
		db_write_word(db, cs->mt->name);
		db_write_time(db, cs->ts);
		db_commit_row(db);
	}
}

/*****************************************************************************/

static void send_memo(sourceinfo_t *si, myuser_t *mu, const char *memo, ...)
{
	service_t *msvs;
	va_list va;
	char buf[BUFSIZE];

	return_if_fail(si != NULL);
	return_if_fail(mu != NULL);
	return_if_fail(memo != NULL);

	va_start(va, memo);
	vsnprintf(buf, BUFSIZE, memo, va);
	va_end(va);

	if ((msvs = service_find("memoserv")) == NULL)
		myuser_notice(chansvs.nick, mu, "%s", buf);
	else
	{
		char cmdbuf[BUFSIZE];

		mowgli_strlcpy(cmdbuf, entity(mu)->name, BUFSIZE);
		mowgli_strlcat(cmdbuf, " ", BUFSIZE);
		mowgli_strlcat(cmdbuf, buf, BUFSIZE);

		command_exec_split(msvs, si, "SEND", cmdbuf, msvs->commands);
	}
}

static void send_group_memo(sourceinfo_t *si, const char *memo, ...)
{
	service_t *msvs;
	va_list va;
	char buf[BUFSIZE];

	return_if_fail(si != NULL);
	return_if_fail(memo != NULL);

	va_start(va, memo);
	vsnprintf(buf, BUFSIZE, memo, va);
	va_end(va);

	if ((msvs = service_find("memoserv")) == NULL)
		return;
	else
	{
		char cmdbuf[BUFSIZE];

		mowgli_strlcpy(cmdbuf, groupmemo, BUFSIZE);
		mowgli_strlcat(cmdbuf, " ", BUFSIZE);
		mowgli_strlcat(cmdbuf, buf, BUFSIZE);

		command_exec_split(msvs, si, "SEND", cmdbuf, msvs->commands);
	}
}

/*****************************************************************************/

/* deny chanserv registrations but turn them into a request */
static void can_register(hook_channel_register_check_t *req)
{
	csreq_t *cs;

	return_if_fail(req != NULL);

	/* no point in moderating registrations from those who have PRIV_CHAN_ADMIN since they can
	 * approve them anyway. --nenolod
	 */
	if (has_priv(req->si, PRIV_CHAN_ADMIN))
		return;

	req->approved++;

	cs = csreq_create(req->name, entity(req->si->smu));
	command_success_nodata(req->si, _("\2%s\2 has channel moderation enabled.  Your request to register \2%s\2 has been received and should be processed shortly."),
			       me.netname, cs->name);

	if (groupmemo != NULL)
		send_group_memo(req->si, "[auto memo] Please register \2%s\2 for me!", req->name);

	logcommand(req->si, CMDLOG_REGISTER, "REGISTER: \2%s\2 (pending)", req->name);
}

/*****************************************************************************/

static void cs_cmd_activate(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	mychan_t *mc;
	csreq_t *cs;
	chanuser_t *cu;
	user_t *u;
	channel_t *c;
	char str[BUFSIZE];
	hook_channel_req_t hdata;
	sourceinfo_t baked_si;
	unsigned int fl;

	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ACTIVATE");
		command_fail(si, fault_needmoreparams, _("Syntax: ACTIVATE <#channel>"));
		return;
	}

	cs = csreq_find(parv[0]);
	if (cs == NULL)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not pending registration."), parv[0]);
		return;
	}

	mu = user(cs->mt);
	if (mu != NULL)
		send_memo(si, mu, "Your registration request for \2%s\2 was approved.", parv[0]);

	c = channel_find(cs->name);
	mc = mychan_add(cs->name);
	mc->registered = CURRTIME;
	mc->used = CURRTIME;
	mc->mlock_on |= (CMODE_NOEXT | CMODE_TOPIC);
	if (c != NULL && c->limit == 0)
		mc->mlock_off |= CMODE_LIMIT;
	if (c != NULL && c->key == NULL)
		mc->mlock_off |= CMODE_KEY;
	mc->flags |= config_options.defcflags;
	slog(LG_DEBUG, "cs_cmd_activate(): defcflags = %d, mc->flags = %d, guard? %s", config_options.defcflags, mc->flags, (mc->flags & MC_GUARD) ? "YES" : "NO");

	chanacs_add(mc, cs->mt, custom_founder_check(), CURRTIME, entity(si->smu));

	command_success_nodata(si, _("\2%s\2 is now registered to \2%s\2."), mc->name, cs->mt->name);

	if (c != NULL && c->ts > 0)
	{
		snprintf(str, sizeof str, "%lu", (unsigned long)c->ts);
		metadata_add(mc, "private:channelts", str);
	}

	if (chansvs.deftemplates != NULL && *chansvs.deftemplates != '\0')
		metadata_add(mc, "private:templates", chansvs.deftemplates);

	if (MOWGLI_LIST_LENGTH(&mu->logins) > 0)
	{
		u = mu->logins.head->data;

		baked_si.su = u;
		baked_si.smu = mu;
		baked_si.service = si->service;

	        hdata.si = &baked_si;
	        hdata.mc = mc;
        	hook_call_channel_register(&hdata);

		if (mc->chan != NULL)
		{
		        /* Allow the hook to override this. */
		        fl = chanacs_source_flags(mc, &baked_si);
		        cu = chanuser_find(mc->chan, u);
		        if (cu == NULL)
	        	        ;
	        	else if (ircd->uses_owner && fl & CA_USEOWNER && fl & CA_AUTOOP &&
		                        !(cu->modes & CSTATUS_OWNER))
        		{
		                modestack_mode_param(si->service->nick, mc->chan, MTYPE_ADD,
        	        	                ircd->owner_mchar[1], CLIENT_NAME(si->su));
        		        cu->modes |= CSTATUS_OWNER;
		        }
        		else if (ircd->uses_protect && fl & CA_USEPROTECT && fl & CA_AUTOOP &&
	        	                !(cu->modes & CSTATUS_PROTECT))
		        {
		                modestack_mode_param(si->service->nick, mc->chan, MTYPE_ADD,
                		                ircd->protect_mchar[1], CLIENT_NAME(si->su));
        	        	cu->modes |= CSTATUS_PROTECT;
		        }
		}
	}

	csreq_destroy(cs);
	logcommand(si, CMDLOG_ADMIN, "ACTIVATE: \2%s\2", parv[0]);
}

static void cs_cmd_reject(sourceinfo_t *si, int parc, char *parv[])
{
	csreq_t *cs;
	myuser_t *mu;
	
	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "REJECT");
		command_fail(si, fault_needmoreparams, _("Syntax: REJECT <#channel>"));
		return;
	}

	cs = csreq_find(parv[0]);
	if (cs == NULL)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not pending registration."), parv[0]);
		return;
	}

	mu = user(cs->mt);
	if (mu != NULL)
		send_memo(si, mu, "Your registration request for \2%s\2 was rejected.", parv[0]);

	csreq_destroy(cs);

	command_success_nodata(si, _("\2%s\2 was rejected."), parv[0]);
	logcommand(si, CMDLOG_ADMIN, "REJECT: \2%s\2", parv[0]);
}

static void cs_cmd_waiting(sourceinfo_t *si, int parc, char *parv[])
{
	mowgli_patricia_iteration_state_t state;
	csreq_t *cs;
	struct tm tm;
	char strfbuf[BUFSIZE];

	MOWGLI_PATRICIA_FOREACH(cs, &state, csreq_list)
	{
		tm = *localtime(&cs->ts);
		strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, &tm);

		command_success_nodata(si, _("\2%s\2 (\2%s\2) [%s (%s ago)]"),
				       cs->name, cs->mt->name, strfbuf, time_ago(cs->ts));
	}

	command_success_nodata(si, _("End of list."));
	logcommand(si, CMDLOG_GET, "WAITING");
}

/*****************************************************************************/

void _modinit(module_t *m)
{
	csreq_list = mowgli_patricia_create(strcasecanon);

	service_named_bind_command("chanserv", &cs_activate);
	service_named_bind_command("chanserv", &cs_reject);
	service_named_bind_command("chanserv", &cs_waiting);

	hook_add_event("channel_can_register");
	hook_add_channel_can_register(can_register);

	hook_add_db_write(csreq_marshal_set);

	add_dupstr_conf_item("REGGROUP", &chansvs.me->conf_table, 0, &groupmemo, NULL);

	db_register_type_handler("CSREQ", csreq_demarshal);
}

void _moddeinit(module_unload_intent_t intent)
{
	mowgli_patricia_destroy(csreq_list, NULL, NULL);

	service_named_unbind_command("chanserv", &cs_activate);
	service_named_unbind_command("chanserv", &cs_reject);
	service_named_unbind_command("chanserv", &cs_waiting);

	hook_del_channel_can_register(can_register);
	hook_del_db_write(csreq_marshal_set);

	del_conf_item("REGGROUP", &chansvs.me->conf_table);

	db_unregister_type_handler("CSREQ");
}
