/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 */

#include "atheme.h"
#include "uplink.h"

typedef struct {
	sourceinfo_t parent;
	sasl_session_t *sess;
} sasl_sourceinfo_t;

static sourceinfo_t *sasl_sourceinfo_create(sasl_session_t *p);
static bool may_impersonate(myuser_t *source_mu, myuser_t *target_mu);
static myuser_t *login_user(sasl_session_t *p);
static void sasl_newuser(hook_user_nick_t *data);
static void sasl_server_eob(server_t *s);
static void delete_stale(void *vptr);
static void mechlist_build_string(char *ptr, size_t buflen);
static void mechlist_do_rebuild(void);

static mowgli_list_t sessions;
static mowgli_list_t mechanisms;
static char mechlist_string[SASL_S2S_MAXLEN];
static bool hide_server_names;

static service_t *saslsvs = NULL;
static mowgli_eventloop_timer_t *delete_stale_timer = NULL;

/* find an existing session by uid */
static sasl_session_t *
find_session(const char *uid)
{
	if (! uid)
		return NULL;

	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, sessions.head)
	{
		sasl_session_t *const p = n->data;

		if (p->uid && strcmp(p->uid, uid) == 0)
			return p;
	}

	return NULL;
}

/* create a new session if it does not already exist */
static sasl_session_t *
make_session(const char *uid, server_t *server)
{
	sasl_session_t *p;

	if (! (p = find_session(uid)))
	{
		p = smalloc(sizeof *p);
		(void) memset(p, 0x00, sizeof *p);

		p->uid = sstrdup(uid);
		p->server = server;

		mowgli_node_t *const n = mowgli_node_create();
		(void) mowgli_node_add(p, n, &sessions);
	}

	return p;
}

/* free a session and all its contents */
static void
destroy_session(sasl_session_t *p)
{
	if (p->flags & ASASL_NEED_LOG && p->username)
	{
		myuser_t *const mu = myuser_find_by_nick(p->username);

		if (mu && ! (ircd->flags & IRCD_SASL_USE_PUID))
		{
			sourceinfo_t *const si = sasl_sourceinfo_create(p);

			(void) logcommand(si, CMDLOG_LOGIN, "LOGIN (session timed out)");
			(void) object_unref(si);
		}
	}

	mowgli_node_t *n, *tn;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, sessions.head)
	{
		if (n->data == p)
		{
			(void) mowgli_node_delete(n, &sessions);
			(void) mowgli_node_free(n);
		}
	}

	if (p->mechptr && p->mechptr->mech_finish)
		(void) p->mechptr->mech_finish(p);

	(void) free(p->username);
	(void) free(p->authzid);
	(void) free(p->certfp);
	(void) free(p->host);
	(void) free(p->buf);
	(void) free(p->uid);
	(void) free(p->ip);
	(void) free(p);
}

static const char *
sasl_format_sourceinfo(sourceinfo_t *si, bool full)
{
	static char result[BUFSIZE];

	sasl_sourceinfo_t *const ssi = (sasl_sourceinfo_t *) si;

	if (full)
		(void) snprintf(result, sizeof result, "SASL/%s:%s[%s]:%s",
		                ssi->sess->uid ? ssi->sess->uid : "?",
		                ssi->sess->host ? ssi->sess->host : "?",
		                ssi->sess->ip ? ssi->sess->ip : "?",
		                ssi->sess->server ? ssi->sess->server->name : "?");
	else
		(void) snprintf(result, sizeof result, "SASL(%s)",
		                ssi->sess->host ? ssi->sess->host : "?");

	return result;
}

static const char *
sasl_get_source_name(sourceinfo_t *si)
{
	static char result[HOSTLEN + NICKLEN + 10];
	char description[BUFSIZE];

	sasl_sourceinfo_t *const ssi = (sasl_sourceinfo_t *) si;

	if (ssi->sess->server && ! hide_server_names)
		(void) snprintf(description, sizeof description, "Unknown user on %s (via SASL)",
		                                                 ssi->sess->server->name);
	else
		(void) mowgli_strlcpy(description, "Unknown user (via SASL)", sizeof description);

	/* we can reasonably assume that si->v is non-null as this is part of the SASL vtable */
	if (si->sourcedesc)
		(void) snprintf(result, sizeof result, "<%s:%s>%s", description, si->sourcedesc,
		                si->smu ? entity(si->smu)->name : "");
	else
		(void) snprintf(result, sizeof result, "<%s>%s", description, si->smu ? entity(si->smu)->name : "");

	return result;
}

static struct sourceinfo_vtable sasl_vtable = {

	.description        = "SASL",
	.format             = sasl_format_sourceinfo,
	.get_source_name    = sasl_get_source_name,
	.get_source_mask    = sasl_get_source_name,
};

static sourceinfo_t *
sasl_sourceinfo_create(sasl_session_t *p)
{
	sasl_sourceinfo_t *const ssi = smalloc(sizeof *ssi);

	(void) object_init(object(ssi), "<sasl sourceinfo>", &free);

	ssi->parent.s = p->server;
	ssi->parent.connection = curr_uplink->conn;

	if (p->host)
		ssi->parent.sourcedesc = p->host;

	ssi->parent.service = saslsvs;
	ssi->parent.v = &sasl_vtable;

	ssi->parent.force_language = language_find("en");
	ssi->sess = p;

	return &ssi->parent;
}

/* find a mechanism by name */
static sasl_mechanism_t *
find_mechanism(char *name)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, mechanisms.head)
	{
		sasl_mechanism_t *const mptr = n->data;

		if (strcmp(mptr->name, name) == 0)
			return mptr;
	}

	(void) slog(LG_DEBUG, "find_mechanism(): cannot find mechanism '%s'!", name);

	return NULL;
}

static void
sasl_server_eob(server_t __attribute__((unused)) *const restrict s)
{
	/* new server online, push mechlist to make sure it's using the current one */
	(void) sasl_mechlist_sts(mechlist_string);
}

static void
mechlist_do_rebuild(void)
{
	(void) mechlist_build_string(mechlist_string, sizeof mechlist_string);

	/* push mechanism list to the network */
	if (me.connected)
		(void) sasl_mechlist_sts(mechlist_string);
}

static void
mechlist_build_string(char *ptr, size_t buflen)
{
	size_t l = 0;
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, mechanisms.head)
	{
		sasl_mechanism_t *const mptr = n->data;

		if (l + strlen(mptr->name) > buflen)
			break;

		(void) strcpy(ptr, mptr->name);

		ptr += strlen(mptr->name);
		*ptr++ = ',';
		l += strlen(mptr->name) + 1;
	}

	if (l)
		ptr--;

	*ptr = 0x00;
}

/* abort an SASL session
 */
static inline void
sasl_session_abort(sasl_session_t *const restrict p)
{
	(void) sasl_sts(p->uid, 'D', "F");
	(void) destroy_session(p);
}

/* output an arbitrary amount of data to the SASL client */
static void
sasl_write(char *target, char *data, size_t length)
{
	size_t last = SASL_S2S_MAXLEN;
	size_t rem = length;

	while (rem)
	{
		const size_t nbytes = (rem > SASL_S2S_MAXLEN) ? SASL_S2S_MAXLEN : rem;

		char out[SASL_S2S_MAXLEN + 1];
		(void) memset(out, 0x00, sizeof out);
		(void) memcpy(out, data, nbytes);
		(void) sasl_sts(target, 'C', out);

		data += nbytes;
		rem -= nbytes;
		last = nbytes;
	}

	/* The end of a packet is indicated by a string not of the maximum length. If last piece was the
	 * maximum length, or if there was no data at all, send an empty string to finish the transaction.
	 */
	if (last == SASL_S2S_MAXLEN)
		(void) sasl_sts(target, 'C', "+");
}

/* given an entire sasl message, advance session by passing data to mechanism
 * and feeding returned data back to client.
 */
static void
sasl_packet(sasl_session_t *p, char *buf, size_t len)
{
	int rc;
	char temp[BUFSIZE];
	metadata_t *md;

	char *out = NULL;
	size_t out_len = 0;

	/* First piece of data in a session is the name of
	 * the SASL mechanism that will be used.
	 */
	if (! p->mechptr)
	{
		char mech[SASL_MECHANISM_MAXLEN];

		if (len >= sizeof mech)
		{
			(void) sasl_session_abort(p);
			return;
		}

		(void) memset(mech, 0x00, sizeof mech);
		(void) memcpy(mech, buf, len);

		if (! (p->mechptr = find_mechanism(mech)))
		{
			(void) sasl_sts(p->uid, 'M', mechlist_string);
			(void) sasl_session_abort(p);
			return;
		}

		if (p->mechptr->mech_start)
			rc = p->mechptr->mech_start(p, &out, &out_len);
		else
			rc = ASASL_MORE;
	}
	else
	{
		size_t tlen = 0;

		if (len == 1 && *buf == '+')
			rc = p->mechptr->mech_step(p, NULL, 0, &out, &out_len);
		else if ((tlen = base64_decode(buf, temp, sizeof temp)) && tlen != (size_t) -1)
			rc = p->mechptr->mech_step(p, temp, tlen, &out, &out_len);
		else
			rc = ASASL_FAIL;

		if (tlen == (size_t) -1)
			(void) slog(LG_ERROR, "%s: base64_decode() failed", __func__);
	}

	/* Some progress has been made, reset timeout. */
	p->flags &= ~ASASL_MARKED_FOR_DELETION;

	if (rc == ASASL_DONE)
	{
		myuser_t *const mu = login_user(p);

		if (mu)
		{
			char *cloak = "*";

			if ((md = metadata_find(mu, "private:usercloak")))
				cloak = md->value;

			if (! (mu->flags & MU_WAITAUTH))
				(void) svslogin_sts(p->uid, "*", "*", cloak, mu);

			(void) sasl_sts(p->uid, 'D', "S");
			/* Will destroy session on introduction of user to net. */
		}
		else
			(void) sasl_session_abort(p);

		return;
	}

	if (rc == ASASL_MORE)
	{
		if (out && out_len)
		{
			const size_t rs = base64_encode(out, out_len, temp, sizeof temp);

			if (rs == (size_t) -1)
			{
				(void) slog(LG_ERROR, "%s: base64_encode() failed", __func__);
				(void) sasl_session_abort(p);
			}
			else
				(void) sasl_write(p->uid, temp, rs);
		}
		else
			(void) sasl_sts(p->uid, 'C', "+");

		(void) free(out);
		return;
	}

	/* If we reach this, they failed SASL auth, so if they were trying
	 * to identify as a specific user, bad_password them.
	 */
	if (p->username)
	{
		myuser_t *const mu = myuser_find_by_nick(p->username);

		if (mu)
		{
			sourceinfo_t *const si = sasl_sourceinfo_create(p);

			(void) logcommand(si, CMDLOG_LOGIN, "failed LOGIN (%s) to \2%s\2 (bad password)",
			                                    p->mechptr->name, entity(mu)->name);
			(void) bad_password(si, mu);
			(void) object_unref(si);
		}
	}

	(void) free(out);
	(void) sasl_session_abort(p);
}

/* interpret an AUTHENTICATE message */
static void
sasl_input(sasl_message_t *smsg)
{
	sasl_session_t *const p = make_session(smsg->uid, smsg->server);

	size_t len = strlen(smsg->parv[0]);
	char *tmpbuf;
	size_t tmplen;

	switch(smsg->mode)
	{
	case 'H':
		/* (H)ost information */
		p->host = sstrdup(smsg->parv[0]);
		p->ip   = sstrdup(smsg->parv[1]);

		if (smsg->parc >= 3 && strcmp(smsg->parv[2], "P") != 0)
			p->tls = true;

		return;

	case 'S':
		/* (S)tart authentication */
		if (smsg->mode == 'S' && smsg->parc >= 2 && strcmp(smsg->parv[0], "EXTERNAL") == 0)
		{
			(void) free(p->certfp);

			p->certfp = sstrdup(smsg->parv[1]);
			p->tls = true;
		}
		/* fallthrough to 'C' */

	case 'C':
		/* (C)lient data */
		if (p->buf == NULL)
		{
			p->buf = (char *)smalloc(len + 1);
			p->p = p->buf;
			p->len = len;
		}
		else
		{
			if (p->len + len >= 8192) /* This is a little much... */
			{
				(void) sasl_sts(p->uid, 'D', "F");
				(void) destroy_session(p);

				return;
			}

			p->buf = (char *)realloc(p->buf, p->len + len + 1);
			p->p = p->buf + p->len;
			p->len += len;
		}

		(void) memcpy(p->p, smsg->parv[0], len);

		/* Messages not exactly 400 bytes are the end of a packet. */
		if (len < 400)
		{
			p->buf[p->len] = 0x00;

			tmpbuf = p->buf;
			tmplen = p->len;

			p->len = 0;
			p->buf = NULL;
			p->p = NULL;

			(void) sasl_packet(p, tmpbuf, tmplen);
			(void) free(tmpbuf);
		}

		return;

	case 'D':
		/* (D)one -- when we receive it, means client abort */
		(void) destroy_session(p);
		return;
	}
}

static bool
may_impersonate(myuser_t *source_mu, myuser_t *target_mu)
{
	/* Allow same (although this function won't get called in that case anyway) */
	if (source_mu == target_mu)
		return true;

	char priv[BUFSIZE] = PRIV_IMPERSONATE_ANY;

	/* Check for wildcard priv */
	if (has_priv_myuser(source_mu, priv))
		return true;

	/* Check for target-operclass specific priv */
	const char *const classname = (target_mu->soper && target_mu->soper->classname) ?
	                                  target_mu->soper->classname : "user";
	(void) snprintf(priv, sizeof priv, PRIV_IMPERSONATE_CLASS_FMT, classname);
	if (has_priv_myuser(source_mu, priv))
		return true;

	/* Check for target-entity specific priv */
	(void) snprintf(priv, sizeof priv, PRIV_IMPERSONATE_ENTITY_FMT, entity(target_mu)->name);
	if (has_priv_myuser(source_mu, priv))
		return true;

	/* Allow modules to check too */
	hook_sasl_may_impersonate_t req = {

		.source_mu = source_mu,
		.target_mu = target_mu,
		.allowed = false,
	};

	(void) hook_call_sasl_may_impersonate(&req);

	return req.allowed;
}

/* authenticated, now double check that their account is ok for login */
static myuser_t *
login_user(sasl_session_t *p)
{
	/* source_mu is the user whose credentials we verified ("authentication id") */
	/* target_mu is the user who will be ultimately logged in ("authorization id") */

	myuser_t *const source_mu = myuser_find_by_nick(p->username);
	if (! source_mu)
		return NULL;

	sourceinfo_t *const si = sasl_sourceinfo_create(p);

	hook_user_login_check_t req = {

		.si = si,
		.mu = source_mu,
		.allowed = true,
	};

	(void) hook_call_user_can_login(&req);

	if (! req.allowed)
	{
		(void) logcommand(si, CMDLOG_LOGIN, "failed LOGIN to \2%s\2 (denied by hook)",
		                                    entity(source_mu)->name);
		return NULL;
	}

	myuser_t *target_mu = source_mu;

	if (p->authzid && *p->authzid)
	{
		if (! (target_mu = myuser_find_by_nick(p->authzid)))
		{
			(void) object_unref(si);
			return NULL;
		}
	}
	else
	{
		(void) free(p->authzid);
		p->authzid = sstrdup(p->username);
	}

	if (metadata_find(source_mu, "private:freeze:freezer"))
	{
		(void) logcommand(si, CMDLOG_LOGIN, "failed LOGIN to \2%s\2 (frozen)", entity(source_mu)->name);
		(void) object_unref(si);
		return NULL;
	}

	if (target_mu != source_mu)
	{
		if (! may_impersonate(source_mu, target_mu))
		{
			(void) logcommand(si, CMDLOG_LOGIN, "denied IMPERSONATE by \2%s\2 to \2%s\2",
			                                    entity(source_mu)->name, entity(target_mu)->name);
			(void) object_unref(si);
			return NULL;
		}

		req.mu = target_mu;
		req.allowed = true;

		(void) hook_call_user_can_login(&req);

		if (! req.allowed)
		{
			(void) logcommand(si, CMDLOG_LOGIN, "failed LOGIN to \2%s\2 (denied by hook)",
			                                    entity(target_mu)->name);
			(void) object_unref(si);
			return NULL;
		}

		if (metadata_find(target_mu, "private:freeze:freezer"))
		{
			(void) logcommand(si, CMDLOG_LOGIN, "failed LOGIN to \2%s\2 (frozen)",
			                                    entity(target_mu)->name);
			(void) object_unref(si);
			return NULL;
		}
	}

	if (MOWGLI_LIST_LENGTH(&target_mu->logins) >= me.maxlogins)
	{
		(void) logcommand(si, CMDLOG_LOGIN, "failed LOGIN to \2%s\2 (too many logins)",
		                                    entity(target_mu)->name);
		(void) object_unref(si);
		return NULL;
	}

	/* Log it with the full n!u@h later */
	p->flags |= ASASL_NEED_LOG;

	/* We just did SASL authentication for a user.  With IRCds which do not
	 * have unique UIDs for users, we will likely be expecting the login
	 * data to be bursted.  As a result, we should give the core a heads'
	 * up that this is going to happen so that hooks will be properly
	 * fired...
	 */
	if (ircd->flags & IRCD_SASL_USE_PUID)
	{
		target_mu->flags &= ~MU_NOBURSTLOGIN;
		target_mu->flags |= MU_PENDINGLOGIN;
	}

	if (target_mu != source_mu)
		(void) logcommand(si, CMDLOG_LOGIN, "allowed IMPERSONATE by \2%s\2 to \2%s\2",
		                                    entity(source_mu)->name, entity(target_mu)->name);

	(void) object_unref(si);
	return target_mu;
}

/* clean up after a user who is finally on the net */
static void
sasl_newuser(hook_user_nick_t *data)
{
	/* If the user has been killed, don't do anything. */
	user_t *const u = data->u;
	if (! u)
		return;

	/* Not concerned unless it's a SASL login. */
	sasl_session_t *const p = find_session(u->uid);
	if (! p)
		return;

	/* We will log it ourselves, if needed */
	p->flags &= ~ASASL_NEED_LOG;

	/* Find the account */
	myuser_t *const mu = p->authzid ? myuser_find_by_nick(p->authzid) : NULL;
	if (! mu)
	{
		(void) notice(saslsvs->nick, u->nick, "Account %s dropped, login cancelled",
		                                      p->authzid ? p->authzid : "??");
		(void) destroy_session(p);

		/* We'll remove their ircd login in handle_burstlogin() */
		return;
	}

	sasl_mechanism_t *const mptr = p->mechptr;

	(void) destroy_session(p);
	(void) myuser_login(saslsvs, u, mu, false);
	(void) logcommand_user(saslsvs, u, CMDLOG_LOGIN, "LOGIN (%s)", mptr->name);
}

/* This function is run approximately once every 30 seconds.
 * It looks for flagged sessions, and deletes them, while
 * flagging all the others. This way stale sessions are deleted
 * after no more than 60 seconds.
 */
static void
delete_stale(void __attribute__((unused)) *const restrict vptr)
{
	mowgli_node_t *n, *tn;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, sessions.head)
	{
		sasl_session_t *const p = n->data;

		if (p->flags & ASASL_MARKED_FOR_DELETION)
		{
			(void) mowgli_node_delete(n, &sessions);
			(void) destroy_session(p);
			(void) mowgli_node_free(n);
		}
		else
			p->flags |= ASASL_MARKED_FOR_DELETION;
	}
}

static void
sasl_mech_register(sasl_mechanism_t *mech)
{
	mowgli_node_t *const node = mowgli_node_create();

	(void) slog(LG_DEBUG, "sasl_mech_register(): registering %s", mech->name);
	(void) mowgli_node_add(mech, node, &mechanisms);
	(void) mechlist_do_rebuild();
}

static void
sasl_mech_unregister(sasl_mechanism_t *mech)
{
	(void) slog(LG_DEBUG, "sasl_mech_unregister(): unregistering %s", mech->name);

	mowgli_node_t *n, *tn;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, sessions.head)
	{
		sasl_session_t *const session = n->data;

		if (session->mechptr == mech)
		{
			(void) slog(LG_DEBUG, "sasl_mech_unregister(): destroying session %s", session->uid);
			(void) destroy_session(session);
		}
	}
	MOWGLI_ITER_FOREACH_SAFE(n, tn, mechanisms.head)
	{
		if (n->data == mech)
		{
			(void) mowgli_node_delete(n, &mechanisms);
			(void) mowgli_node_free(n);
			(void) mechlist_do_rebuild();

			break;
		}
	}
}

/* main services client routine */
static void
saslserv(sourceinfo_t *si, int parc, char *parv[])
{
	/* this should never happen */
	if (parv[0][0] == '&')
	{
		(void) slog(LG_ERROR, "services(): got parv with local channel: %s", parv[0]);
		return;
	}

	/* make a copy of the original for debugging */
	char orig[BUFSIZE];
	(void) mowgli_strlcpy(orig, parv[parc - 1], sizeof orig);

	/* lets go through this to get the command */
	char *const cmd = strtok(parv[parc - 1], " ");
	char *const text = strtok(NULL, "");

	if (! cmd)
		return;

	if (*orig == '\001')
	{
		(void) handle_ctcp_common(si, cmd, text);
		return;
	}

	(void) command_fail(si, fault_noprivs, "This service exists to identify connecting clients to the network. "
	                                       "It has no public interface.");
}

static void
mod_init(module_t __attribute__((unused)) *const restrict m)
{
	(void) hook_add_event("sasl_input");
	(void) hook_add_sasl_input(&sasl_input);
	(void) hook_add_event("user_add");
	(void) hook_add_user_add(&sasl_newuser);
	(void) hook_add_event("server_eob");
	(void) hook_add_server_eob(&sasl_server_eob);
	(void) hook_add_event("sasl_may_impersonate");
	(void) hook_add_event("user_can_login");

	delete_stale_timer = mowgli_timer_add(base_eventloop, "sasl_delete_stale", &delete_stale, NULL, 30);
	saslsvs = service_add("saslserv", &saslserv);
	authservice_loaded++;

	(void) add_bool_conf_item("HIDE_SERVER_NAMES", &saslsvs->conf_table, 0, &hide_server_names, false);
}

static void
mod_deinit(const module_unload_intent_t __attribute__((unused)) intent)
{
	(void) hook_del_sasl_input(&sasl_input);
	(void) hook_del_user_add(&sasl_newuser);
	(void) hook_del_server_eob(&sasl_server_eob);

	(void) mowgli_timer_destroy(base_eventloop, delete_stale_timer);

	(void) del_conf_item("HIDE_SERVER_NAMES", &saslsvs->conf_table);

        if (saslsvs)
		(void) service_delete(saslsvs);

	authservice_loaded--;

	if (sessions.head)
		(void) slog(LG_ERROR, "saslserv/main: shutting down with a non-empty session list; "
		                      "a mechanism did not unregister itself! (BUG)");
}

/* This structure is imported by SASL mechanism modules */
const sasl_core_functions_t sasl_core_functions = {

	.mech_register      = &sasl_mech_register,
	.mech_unregister    = &sasl_mech_unregister,
};

SIMPLE_DECLARE_MODULE_V1("saslserv/main", MODULE_UNLOAD_CAPABILITY_OK)
