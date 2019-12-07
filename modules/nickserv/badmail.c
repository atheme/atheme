/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010-2011 William Pitcock <nenolod -at- nenolod.net>
 *
 * Allows banning certain email addresses from registering.
 */

#include <atheme.h>

struct badmail
{
	char *mail;
	time_t mail_ts;
	char *creator;
	char *reason;
};

static mowgli_list_t ns_maillist;

static void
write_bedb(struct database_handle *db)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, ns_maillist.head)
	{
		struct badmail *l = n->data;

		db_start_row(db, "BE");
		db_write_word(db, l->mail);
		db_write_time(db, l->mail_ts);
		db_write_word(db, l->creator);
		db_write_str(db, l->reason);
		db_commit_row(db);
	}
}

static void
db_h_be(struct database_handle *db, const char *type)
{
	const char *mail = db_sread_word(db);
	time_t mail_ts = db_sread_time(db);
	const char *creator = db_sread_word(db);
	const char *reason = db_sread_str(db);

	struct badmail *const l = smalloc(sizeof *l);
	l->mail = sstrdup(mail);
	l->mail_ts = mail_ts;
	l->creator = sstrdup(creator);
	l->reason = sstrdup(reason);
	mowgli_node_add(l, mowgli_node_create(), &ns_maillist);
}

static void
check_registration(struct hook_user_register_check *hdata)
{
	mowgli_node_t *n;
	struct badmail *l;

	if (hdata->approved)
		return;

	MOWGLI_ITER_FOREACH(n, ns_maillist.head)
	{
		l = n->data;

		if (!match(l->mail, hdata->email))
		{
			command_fail(hdata->si, fault_noprivs, _("Sorry, we do not accept registrations with email addresses from that domain. Use another address."));
			hdata->approved = 1;
			slog(LG_INFO, "REGISTER:BADEMAIL: %s to \2%s\2 by \2%s\2",
					hdata->account, hdata->email,
					hdata->si->su != NULL ? hdata->si->su->nick : get_source_name(hdata->si));
			return;
		}
	}
}

static void
ns_cmd_badmail(struct sourceinfo *si, int parc, char *parv[])
{
	char *action = parv[0];
	char *email = parv[1];
	char *reason = parv[2];
	mowgli_node_t *n, *tn;
	struct badmail *l;

	if (!action)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "BADMAIL");
		command_fail(si, fault_needmoreparams, _("Syntax: BADMAIL ADD|DEL|LIST|TEST [parameters]"));
		return;
	}

	if (!strcasecmp("ADD", action))
	{
		if (!email || !reason)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "BADMAIL");
			command_fail(si, fault_needmoreparams, _("Syntax: BADMAIL ADD <email> <reason>"));
			return;
		}

		if (si->smu == NULL)
		{
			command_fail(si, fault_noprivs, STR_NOT_LOGGED_IN);
			return;
		}

		// search for it
		MOWGLI_ITER_FOREACH(n, ns_maillist.head)
		{
			l = n->data;

			if (!irccasecmp(l->mail, email))
			{
				command_success_nodata(si, _("Email \2%s\2 has already been banned."), email);
				return;
			}
		}

		l = smalloc(sizeof *l);
		l->mail = sstrdup(email);
		l->mail_ts = CURRTIME;;
		l->creator = sstrdup(get_source_name(si));
		l->reason = sstrdup(reason);

		logcommand(si, CMDLOG_ADMIN, "BADMAIL:ADD: \2%s\2 (Reason: \2%s\2)", email, reason);

		n = mowgli_node_create();
		mowgli_node_add(l, n, &ns_maillist);

		command_success_nodata(si, _("You have banned email address \2%s\2."), email);
		return;
	}
	else if (!strcasecmp("DEL", action))
	{
		if (!email)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "BADMAIL");
			command_fail(si, fault_needmoreparams, _("Syntax: BADMAIL DEL <email>"));
			return;
		}

		MOWGLI_ITER_FOREACH_SAFE(n, tn, ns_maillist.head)
		{
			l = n->data;

			if (!irccasecmp(l->mail, email))
			{
				logcommand(si, CMDLOG_ADMIN, "BADMAIL:DEL: \2%s\2", l->mail);

				mowgli_node_delete(n, &ns_maillist);

				sfree(l->mail);
				sfree(l->creator);
				sfree(l->reason);
				sfree(l);

				command_success_nodata(si, _("You have unbanned email address \2%s\2."), email);
				return;
			}
		}
		command_success_nodata(si, _("Email pattern \2%s\2 not found in badmail database."), email);
		return;
	}
	else if (!strcasecmp("LIST", action))
	{
		char buf[BUFSIZE];
		struct tm *tm;
		unsigned int count = 0;

		MOWGLI_ITER_FOREACH(n, ns_maillist.head)
		{
			l = n->data;

			if ((!email) || !match(email, l->mail))
			{
				count++;
				tm = localtime(&l->mail_ts);
				strftime(buf, BUFSIZE, TIME_FORMAT, tm);
				command_success_nodata(si, _("Email: \2%s\2, Reason: \2%s\2 (%s - %s)"),
					l->mail, l->reason, l->creator, buf);
			}
		}
		if (email && !count)
			command_success_nodata(si, _("No entries matching pattern \2%s\2 found in badmail database."), email);
		else
			command_success_nodata(si, _("End of list."));

		if (email)
			logcommand(si, CMDLOG_GET, "BADMAIL:LIST: \2%s\2 (\2%u\2 matches)", email, count);
		else
			logcommand(si, CMDLOG_GET, "BADMAIL:LIST (\2%u\2 matches)", count);
		return;
	}
	else if (!strcasecmp("TEST", action))
	{
		char buf[BUFSIZE];
		struct tm *tm;
		unsigned int count = 0;

		if (!email)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "BADMAIL");
			command_fail(si, fault_needmoreparams, _("Syntax: BADMAIL TEST <email>"));
			return;
		}

		MOWGLI_ITER_FOREACH(n, ns_maillist.head)
		{
			l = n->data;

			if (!match(l->mail, email))
			{
				count++;
				tm = localtime(&l->mail_ts);
				strftime(buf, BUFSIZE, TIME_FORMAT, tm);
				command_success_nodata(si, _("Email: \2%s\2, Reason: \2%s\2 (%s - %s)"),
					l->mail, l->reason, l->creator, buf);
			}
		}
		if (count)
			command_success_nodata(si, ngettext(N_("%u badmail pattern disallowing \2%s\2 found."),
			                                    N_("%u badmail patterns disallowing \2%s\2 found."),
			                                    count), count, email);
		else
			command_success_nodata(si, _("\2%s\2 is not listed in the badmail database."), email);

		logcommand(si, CMDLOG_GET, "BADMAIL:TEST: \2%s\2 (\2%u\2 matches)", email, count);
		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "BADMAIL");
		return;
	}
}

static struct command ns_badmail = {
	.name           = "BADMAIL",
	.desc           = N_("Disallows registrations from certain email addresses."),
	.access         = PRIV_USER_ADMIN,
	.maxparc        = 3,
	.cmd            = &ns_cmd_badmail,
	.help           = { .path = "nickserv/badmail" },
};

static void
mod_init(struct module *const restrict m)
{
	if (!module_find_published("backend/opensex"))
	{
		slog(LG_INFO, "Module %s requires use of the OpenSEX database backend, refusing to load.", m->name);
		m->mflags |= MODFLAG_FAIL;
		return;
	}

	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main")

	hook_add_user_can_register(check_registration);
	hook_add_db_write(write_bedb);

	db_register_type_handler("BE", db_h_be);

	service_named_bind_command("nickserv", &ns_badmail);

	m->mflags |= MODFLAG_DBHANDLER;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("nickserv/badmail", MODULE_UNLOAD_CAPABILITY_NEVER)
