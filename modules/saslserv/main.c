/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2006-2015 Atheme Project (http://atheme.org/)
 * Copyright (C) 2017-2019 Atheme Development Group (https://atheme.github.io/)
 *
 * This file contains the main() routine.
 */

#include <atheme.h>

#ifndef MINIMUM
#  define MINIMUM(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define ASASL_OUTFLAGS_WIPE_FREE_BUF    (ASASL_OUTFLAG_WIPE_BUF | ASASL_OUTFLAG_FREE_BUF)
#define LOGIN_CANCELLED_STR             "There was a problem logging you in; login cancelled"

static mowgli_list_t sasl_sessions;
static mowgli_list_t sasl_mechanisms;
static char sasl_mechlist_string[SASL_S2S_MAXLEN_ATONCE_B64];
static bool sasl_hide_server_names;

static mowgli_eventloop_timer_t *sasl_delete_stale_timer = NULL;
static struct service *saslsvs = NULL;

static const char *
sasl_format_sourceinfo(struct sourceinfo *const restrict si, const bool full)
{
	static char result[BUFSIZE];

	const struct sasl_sourceinfo *const ssi = (const struct sasl_sourceinfo *) si;

	if (full)
		(void) snprintf(result, sizeof result, "SASL/%s:%s[%s]:%s",
		                *ssi->sess->uid ? ssi->sess->uid : "?",
		                ssi->sess->host ? ssi->sess->host : "?",
		                ssi->sess->ip ? ssi->sess->ip : "?",
		                ssi->sess->server ? ssi->sess->server->name : "?");
	else
		(void) snprintf(result, sizeof result, "SASL(%s)",
		                ssi->sess->host ? ssi->sess->host : "?");

	return result;
}

static const char *
sasl_get_source_name(struct sourceinfo *const restrict si)
{
	static char result[HOSTLEN + 1 + NICKLEN + 11];
	char description[BUFSIZE];

	const struct sasl_sourceinfo *const ssi = (const struct sasl_sourceinfo *) si;

	if (ssi->sess->server && ! sasl_hide_server_names)
		(void) snprintf(description, sizeof description, "Unknown user on %s (via SASL)",
		                                                 ssi->sess->server->name);
	else
		(void) mowgli_strlcpy(description, "Unknown user (via SASL)", sizeof description);

	// we can reasonably assume that si->v is non-null as this is part of the SASL vtable
	if (si->sourcedesc)
		(void) snprintf(result, sizeof result, "<%s:%s>%s", description, si->sourcedesc,
		                si->smu ? entity(si->smu)->name : "");
	else
		(void) snprintf(result, sizeof result, "<%s>%s", description, si->smu ? entity(si->smu)->name : "");

	return result;
}

static void
sasl_sourceinfo_recreate(struct sasl_session *const restrict p)
{
	static struct sourceinfo_vtable sasl_vtable = {

		.description        = "SASL",
		.format             = sasl_format_sourceinfo,
		.get_source_name    = sasl_get_source_name,
		.get_source_mask    = sasl_get_source_name,
	};

	if (p->si)
		(void) atheme_object_unref(p->si);

	struct sasl_sourceinfo *const ssi = smalloc(sizeof *ssi);

	(void) atheme_object_init(atheme_object(ssi), "<sasl sourceinfo>", &sfree);

	ssi->parent.s = p->server;
	ssi->parent.connection = curr_uplink->conn;

	if (p->host)
		ssi->parent.sourcedesc = p->host;

	ssi->parent.service = saslsvs;
	ssi->parent.v = &sasl_vtable;

	ssi->parent.force_language = language_find("en");
	ssi->sess = p;

	p->si = &ssi->parent;
}

static struct sasl_session *
sasl_session_find(const char *const restrict uid)
{
	if (! uid || ! *uid)
		return NULL;

	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, sasl_sessions.head)
	{
		struct sasl_session *const p = n->data;

		if (strcmp(p->uid, uid) == 0)
			return p;
	}

	return NULL;
}

static struct sasl_session *
sasl_session_find_or_make(const struct sasl_message *const restrict smsg)
{
	struct sasl_session *p;

	if (! (p = sasl_session_find(smsg->uid)))
	{
		p = smalloc(sizeof *p);

		p->server = smsg->server;

		(void) mowgli_strlcpy(p->uid, smsg->uid, sizeof p->uid);
		(void) mowgli_node_add(p, &p->node, &sasl_sessions);
	}

	return p;
}

static const struct sasl_mechanism *
sasl_mechanism_find(const char *const restrict name)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, sasl_mechanisms.head)
	{
		const struct sasl_mechanism *const mptr = n->data;

		if (strcmp(mptr->name, name) == 0)
			return mptr;
	}

	(void) slog(LG_DEBUG, "%s: cannot find mechanism '%s'!", MOWGLI_FUNC_NAME, name);

	return NULL;
}

static void
sasl_server_eob(struct server ATHEME_VATTR_UNUSED *const restrict s)
{
	(void) sasl_mechlist_sts(sasl_mechlist_string);
}

static void
sasl_mechlist_string_build(const struct sasl_session *const restrict p, const struct myuser *const restrict mu,
                           const char **const restrict avoid)
{
	char buf[sizeof sasl_mechlist_string];
	char *bufptr = buf;
	size_t written = 0;
	mowgli_node_t *n;

	(void) memset(buf, 0x00, sizeof buf);

	MOWGLI_ITER_FOREACH(n, sasl_mechanisms.head)
	{
		const struct sasl_mechanism *const mptr = n->data;
		bool in_avoid_list = false;

		continue_if_fail(mptr != NULL);

		for (size_t i = 0; avoid != NULL && avoid[i] != NULL; i++)
		{
			if (strcmp(mptr->name, avoid[i]) == 0)
			{
				in_avoid_list = true;
				break;
			}
		}

		if (in_avoid_list || (mptr->password_based && mu != NULL && (mu->flags & MU_NOPASSWORD)))
			continue;

		const size_t namelen = strlen(mptr->name);

		if (written + namelen >= sizeof buf)
			break;

		(void) memcpy(bufptr, mptr->name, namelen);

		bufptr += namelen;
		*bufptr++ = ',';
		written += namelen + 1;
	}

	if (written)
		*(--bufptr) = 0x00;

	if (p)
		(void) sasl_sts(p->uid, 'M', buf);
	else
		(void) memcpy(sasl_mechlist_string, buf, sizeof buf);
}

static void
sasl_mechlist_do_rebuild(void)
{
	(void) sasl_mechlist_string_build(NULL, NULL, NULL);

	if (me.connected)
		(void) sasl_mechlist_sts(sasl_mechlist_string);
}

static bool
sasl_may_impersonate(struct myuser *const source_mu, struct myuser *const target_mu)
{
	if (source_mu == target_mu)
		return true;

	char priv[BUFSIZE] = PRIV_IMPERSONATE_ANY;

	// Check for wildcard priv
	if (has_priv_myuser(source_mu, priv))
		return true;

	// Check for target-operclass specific priv
	const char *const classname = (target_mu->soper && target_mu->soper->classname) ?
	                                  target_mu->soper->classname : "user";
	(void) snprintf(priv, sizeof priv, PRIV_IMPERSONATE_CLASS_FMT, classname);
	if (has_priv_myuser(source_mu, priv))
		return true;

	// Check for target-entity specific priv
	(void) snprintf(priv, sizeof priv, PRIV_IMPERSONATE_ENTITY_FMT, entity(target_mu)->name);
	if (has_priv_myuser(source_mu, priv))
		return true;

	// Allow modules to check too
	struct hook_sasl_may_impersonate req = {

		.source_mu = source_mu,
		.target_mu = target_mu,
		.allowed = false,
	};

	(void) hook_call_sasl_may_impersonate(&req);

	return req.allowed;
}

static struct myuser *
sasl_user_can_login(struct sasl_session *const restrict p)
{
	// source_mu is the user whose credentials we verified ("authentication id" / authcid)
	// target_mu is the user who will be ultimately logged in ("authorization id" / authzid)
	struct myuser *source_mu;
	struct myuser *target_mu;

	if (! *p->authceid || ! (source_mu = myuser_find_uid(p->authceid)))
		return NULL;

	if (! *p->authzeid)
	{
		target_mu = source_mu;

		(void) mowgli_strlcpy(p->authzid, p->authcid, sizeof p->authzid);
		(void) mowgli_strlcpy(p->authzeid, p->authceid, sizeof p->authzeid);
	}
	else if (! (target_mu = myuser_find_uid(p->authzeid)))
		return NULL;

	if (! sasl_may_impersonate(source_mu, target_mu))
	{
		(void) logcommand(p->si, CMDLOG_LOGIN, "denied IMPERSONATE by \2%s\2 to \2%s\2",
		                                       entity(source_mu)->name, entity(target_mu)->name);
		return NULL;
	}

	if (user_loginmaxed(target_mu))
	{
		(void) logcommand(p->si, CMDLOG_LOGIN, "failed LOGIN to \2%s\2 (too many logins)",
		                                       entity(target_mu)->name);
		return NULL;
	}

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
		(void) logcommand(p->si, CMDLOG_LOGIN, "allowed IMPERSONATE by \2%s\2 to \2%s\2",
		                                       entity(source_mu)->name, entity(target_mu)->name);

	return target_mu;
}

static void
sasl_session_reset(struct sasl_session *const restrict p)
{
	if (p->mechptr && p->mechptr->mech_finish)
		(void) p->mechptr->mech_finish(p);
	p->mechptr = NULL;

	struct user *const u = user_find(p->uid);
	if (u)
		// If the user is still on the network, allow them to use NickServ IDENTIFY/LOGIN again
		u->flags &= ~UF_DOING_SASL;
}

static void
sasl_session_destroy(struct sasl_session *const restrict p)
{
	mowgli_node_t *n;

	sasl_session_reset(p);

	MOWGLI_ITER_FOREACH(n, sasl_sessions.head)
	{
		if (n == &p->node && n->data == p)
		{
			(void) mowgli_node_delete(n, &sasl_sessions);
			break;
		}
	}

	if (p->si)
		(void) atheme_object_unref(p->si);

	(void) sfree(p->certfp);
	(void) sfree(p->host);
	(void) sfree(p->buf);
	(void) sfree(p->ip);
	(void) sfree(p);
}

static void
sasl_session_reset_or_destroy(struct sasl_session *const restrict p)
{
	if (p->pendingeid[0] == '\0')
		sasl_session_destroy(p);
	else
		sasl_session_reset(p);
}

static inline void
sasl_session_abort(struct sasl_session *const restrict p)
{
	(void) sasl_sts(p->uid, 'D', "F");
	(void) sasl_session_reset_or_destroy(p);
}

static bool
sasl_session_success(struct sasl_session *const restrict p, struct myuser *const restrict mu, const bool destroy)
{
	/* Only burst an account name and vhost if the user has verified their e-mail address;
	 * this prevents spambots from creating accounts to join registered-user-only channels
	 */
	if (! (mu->flags & MU_WAITAUTH))
	{
		const struct metadata *const md = metadata_find(mu, "private:usercloak");
		const char *const cloak = ((md != NULL) ? md->value : "*");

		(void) svslogin_sts(p->uid, "*", "*", cloak, mu);
	}

	(void) sasl_sts(p->uid, 'D', "S");

	if (destroy)
		(void) sasl_session_destroy(p);

	return true;
}

static bool
sasl_handle_login(struct sasl_session *const restrict p, struct user *const u, struct myuser *mu)
{
	bool was_killed = false;
	char pendingeid[sizeof p->pendingeid];

	mowgli_strlcpy(pendingeid, p->pendingeid, sizeof pendingeid);
	p->pendingeid[0] = '\0';

	// Find the account if necessary
	if (! mu)
	{
		if (! *pendingeid)
		{
			(void) slog(LG_INFO, "%s: session for '%s' without a pendingeid (BUG)",
			                     MOWGLI_FUNC_NAME, u->nick);
			(void) notice(saslsvs->nick, u->nick, LOGIN_CANCELLED_STR);
			return false;
		}

		if (! (mu = myuser_find_uid(pendingeid)))
		{
			if (*p->authzid)
				(void) notice(saslsvs->nick, u->nick, "Account %s dropped; login cancelled",
				                                      p->authzid);
			else
				(void) notice(saslsvs->nick, u->nick, "Account dropped; login cancelled");

			return false;
		}
	}

	// If the user is already logged in, and not to the same account, log them out first
	if (u->myuser && u->myuser != mu)
	{
		if (is_soper(u->myuser))
			(void) logcommand_user(saslsvs, u, CMDLOG_ADMIN, "DESOPER: \2%s\2 as \2%s\2",
			                                                 u->nick, entity(u->myuser)->name);

		(void) logcommand_user(saslsvs, u, CMDLOG_LOGIN, "LOGOUT");

		if (! (was_killed = ircd_on_logout(u, entity(u->myuser)->name)))
		{
			mowgli_node_t *n;

			MOWGLI_ITER_FOREACH(n, u->myuser->logins.head)
			{
				if (n->data == u)
				{
					(void) mowgli_node_delete(n, &u->myuser->logins);
					(void) mowgli_node_free(n);
					break;
				}
			}

			u->myuser = NULL;
		}
	}

	// If they were not killed above, log them in now
	if (! was_killed)
	{
		if (u->myuser != mu)
		{
			// If they're not logged in, or logging in to a different account, do a full login
			(void) myuser_login(saslsvs, u, mu, false);
			(void) logcommand_user(saslsvs, u, CMDLOG_LOGIN, "LOGIN (%s)", p->mechptr->name);
		}
		else
		{
			// Otherwise, just update login time ...
			mu->lastlogin = CURRTIME;
			(void) logcommand_user(saslsvs, u, CMDLOG_LOGIN, "REAUTHENTICATE (%s)", p->mechptr->name);
		}
	}

	return true;
}

static enum sasl_mechanism_result ATHEME_FATTR_WUR
sasl_process_input(struct sasl_session *const restrict p, char *const restrict buf, const size_t len,
                   struct sasl_output_buf *const restrict outbuf)
{
	// A single + character is not data at all -- invoke mech_step without an input buffer
	if (*buf == '+' && len == 1)
		return p->mechptr->mech_step(p, NULL, outbuf);

	unsigned char decbuf[SASL_S2S_MAXLEN_TOTAL_RAW + 1];
	const size_t declen = base64_decode(buf, decbuf, SASL_S2S_MAXLEN_TOTAL_RAW);

	if (declen == BASE64_FAIL)
	{
		(void) slog(LG_DEBUG, "%s: base64_decode() failed", MOWGLI_FUNC_NAME);
		return ASASL_MRESULT_ERROR;
	}

	/* Account for the fact that the client may have sent whitespace; our
	 * decoder is tolerant of whitespace and will skip over it    -- amdj
	 */
	if (declen == 0)
		return p->mechptr->mech_step(p, NULL, outbuf);

	unsigned int inflags = ASASL_INFLAG_NONE;
	const struct sasl_input_buf inbuf = {
		.buf    = decbuf,
		.len    = declen,
		.flags  = &inflags,
	};

	// Ensure input is NULL-terminated for modules that want to process the data as a string
	decbuf[declen] = 0x00;

	// Pass the data to the mechanism
	const enum sasl_mechanism_result rc = p->mechptr->mech_step(p, &inbuf, outbuf);

	// The mechanism instructed us to wipe the input data now that it has been processed
	if (inflags & ASASL_INFLAG_WIPE_BUF)
	{
		/* If we got here, the bufferred base64-encoded input data is either in a
		 * dedicated buffer (buf == p->buf && len == p->len) or directly from a
		 * parv[] inside struct sasl_message. Either way buf is mutable.    -- amdj
		 */
		(void) smemzero(buf, len);          // Erase the base64-encoded input data
		(void) smemzero(decbuf, declen);    // Erase the base64-decoded input data
	}

	return rc;
}

static bool ATHEME_FATTR_WUR
sasl_process_output(struct sasl_session *const restrict p, struct sasl_output_buf *const restrict outbuf)
{
	char encbuf[SASL_S2S_MAXLEN_TOTAL_B64 + 1];
	const size_t enclen = base64_encode(outbuf->buf, outbuf->len, encbuf, sizeof encbuf);

	if ((outbuf->flags & ASASL_OUTFLAGS_WIPE_FREE_BUF) == ASASL_OUTFLAGS_WIPE_FREE_BUF)
		// The mechanism instructed us to wipe and free the output data now that it has been encoded
		(void) smemzerofree(outbuf->buf, outbuf->len);
	else if (outbuf->flags & ASASL_OUTFLAG_WIPE_BUF)
		// The mechanism instructed us to wipe the output data now that it has been encoded
		(void) smemzero(outbuf->buf, outbuf->len);
	else if (outbuf->flags & ASASL_OUTFLAG_FREE_BUF)
		// The mechanism instructed us to free the output data now that it has been encoded
		(void) sfree(outbuf->buf);

	outbuf->buf = NULL;
	outbuf->len = 0;

	if (enclen == BASE64_FAIL)
	{
		(void) slog(LG_ERROR, "%s: base64_encode() failed", MOWGLI_FUNC_NAME);
		return false;
	}

	char *encbufptr = encbuf;
	size_t encbuflast = SASL_S2S_MAXLEN_ATONCE_B64;

	for (size_t encbufrem = enclen; encbufrem != 0; /* No action */)
	{
		char encbufpart[SASL_S2S_MAXLEN_ATONCE_B64 + 1];
		const size_t encbufptrlen = MINIMUM(SASL_S2S_MAXLEN_ATONCE_B64, encbufrem);

		(void) memset(encbufpart, 0x00, sizeof encbufpart);
		(void) memcpy(encbufpart, encbufptr, encbufptrlen);
		(void) sasl_sts(p->uid, 'C', encbufpart);

		// The mechanism instructed us to wipe the output data now that it has been transmitted
		if (outbuf->flags & ASASL_OUTFLAG_WIPE_BUF)
		{
			(void) smemzero(encbufpart, encbufptrlen);
			(void) smemzero(encbufptr, encbufptrlen);
		}

		encbufptr += encbufptrlen;
		encbufrem -= encbufptrlen;
		encbuflast = encbufptrlen;
	}

	/* The end of a packet is indicated by a string not of the maximum length. If the last string
	 * was the maximum length, send another, empty string, to advance the session.    -- amdj
	 */
	if (encbuflast == SASL_S2S_MAXLEN_ATONCE_B64)
		(void) sasl_sts(p->uid, 'C', "+");

	return true;
}

/* given an entire sasl message, advance session by passing data to mechanism
 * and feeding returned data back to client.
 */
static bool ATHEME_FATTR_WUR
sasl_process_packet(struct sasl_session *const restrict p, char *const restrict buf, const size_t len)
{
	struct sasl_output_buf outbuf = {
		.buf    = NULL,
		.len    = 0,
		.flags  = ASASL_OUTFLAG_NONE,
	};

	enum sasl_mechanism_result rc;
	bool have_responded = false;

	if (! p->mechptr && ! len)
	{
		// First piece of data in a session is the name of the SASL mechanism that will be used
		if (! (p->mechptr = sasl_mechanism_find(buf)))
		{
			(void) sasl_sts(p->uid, 'M', sasl_mechlist_string);
			return false;
		}

		(void) sasl_sourceinfo_recreate(p);

		if (p->mechptr->mech_start)
			rc = p->mechptr->mech_start(p, &outbuf);
		else
			rc = ASASL_MRESULT_CONTINUE;
	}
	else if (! p->mechptr)
	{
		(void) slog(LG_DEBUG, "%s: session has no mechanism?", MOWGLI_FUNC_NAME);
		return false;
	}
	else
	{
		rc = sasl_process_input(p, buf, len, &outbuf);
	}

	if (outbuf.buf && outbuf.len)
	{
		if (! sasl_process_output(p, &outbuf))
			return false;

		have_responded = true;
	}

	// Some progress has been made, reset timeout.
	p->flags &= ~ASASL_SFLAG_MARKED_FOR_DELETION;

	switch (rc)
	{
		case ASASL_MRESULT_CONTINUE:
		{
			if (! have_responded)
				/* We want more data from the client, but we haven't sent any of our own.
				 * Send an empty string to advance the session.    -- amdj
				 */
				(void) sasl_sts(p->uid, 'C', "+");

			return true;
		}

		case ASASL_MRESULT_SUCCESS:
		{
			struct user *const u = user_find(p->uid);
			struct myuser *const mu = sasl_user_can_login(p);

			if (! mu)
			{
				if (u)
					(void) notice(saslsvs->nick, u->nick, LOGIN_CANCELLED_STR);

				return false;
			}

			(void) mowgli_strlcpy(p->pendingeid, p->authzeid, sizeof p->pendingeid);

			/* If the user is already on the network, attempt to log them in immediately.
			 * Otherwise, we will log them in on introduction of user to network
			 */
			if (u && ! sasl_handle_login(p, u, mu))
				return false;

			return sasl_session_success(p, mu, (u != NULL));
		}

		case ASASL_MRESULT_FAILURE:
		{
			if (*p->authceid)
			{
				/* If we reach this, they failed SASL auth, so if they were trying
				 * to authenticate as a specific user, run bad_password() on them.
				 */
				struct myuser *const mu = myuser_find_uid(p->authceid);

				if (! mu)
					return false;

				/* We might have more information to construct a more accurate sourceinfo now?
				 * TODO: Investigate whether this is necessary
				 */
				(void) sasl_sourceinfo_recreate(p);

				(void) logcommand(p->si, CMDLOG_LOGIN, "failed LOGIN (%s) to \2%s\2 (bad password)",
				                                       p->mechptr->name, entity(mu)->name);
				(void) bad_password(p->si, mu);
			}

			return false;
		}

		case ASASL_MRESULT_ERROR:
			return false;
	}

	/* This is only here to keep GCC happy -- Clang can see that the switch() handles all legal
	 * values of the enumeration, and so knows that this function will never get to this point;
	 * GCC is dumb, and warns that control reaches the end of this non-void function.    -- amdj
	 */
	return false;
}

static bool ATHEME_FATTR_WUR
sasl_process_buffer(struct sasl_session *const restrict p)
{
	// Ensure the buffer is NULL-terminated so that base64_decode() doesn't overrun it
	p->buf[p->len] = 0x00;

	if (! sasl_process_packet(p, p->buf, p->len))
		return false;

	(void) sfree(p->buf);

	p->buf = NULL;
	p->len = 0;

	return true;
}

static void
sasl_input_hostinfo(const struct sasl_message *const restrict smsg, struct sasl_session *const restrict p)
{
	p->host = sstrdup(smsg->parv[0]);
	p->ip   = sstrdup(smsg->parv[1]);

	if (smsg->parc >= 3 && strcmp(smsg->parv[2], "P") != 0)
		p->flags |= ASASL_SFLAG_CLIENT_SECURE;
}

static bool ATHEME_FATTR_WUR
sasl_input_startauth(const struct sasl_message *const restrict smsg, struct sasl_session *const restrict p)
{
	if (strcmp(smsg->parv[0], "EXTERNAL") == 0)
	{
		if (smsg->parc < 2)
		{
			(void) slog(LG_DEBUG, "%s: client %s starting EXTERNAL authentication without a "
			                      "fingerprint", MOWGLI_FUNC_NAME, p->uid);
			return false;
		}

		(void) sfree(p->certfp);

		p->certfp = sstrdup(smsg->parv[1]);
		p->flags |= ASASL_SFLAG_CLIENT_SECURE;
	}

	struct user *const u = user_find(p->uid);

	if (u && u->myuser)
	{
		/* If the user is already on the network, they're doing an IRCv3.2 SASL
		 * reauthentication. This means that if the user is logged in, we need
		 * to call the user_can_logout hooks and maybe abort the exchange now.
		 */
		(void) slog(LG_DEBUG, "%s: user %s ('%s') is logged in as '%s' -- executing user_can_logout hooks",
		                      MOWGLI_FUNC_NAME, p->uid, u->nick, entity(u->myuser)->name);

		struct hook_user_logout_check req = {
			.si      = p->si,
			.u       = u,
			.allowed = true,
			.relogin = true,
		};

		(void) hook_call_user_can_logout(&req);

		if (! req.allowed)
		{
			(void) notice(saslsvs->nick, u->nick,
			              "You cannot log out \2%s\2 because the server configuration disallows it.",
			              entity(u->myuser)->name);
			return false;
		}
	}

	if (u)
		u->flags |= UF_DOING_SASL;

	return sasl_process_packet(p, smsg->parv[0], 0);
}

static bool ATHEME_FATTR_WUR
sasl_input_clientdata(const struct sasl_message *const restrict smsg, struct sasl_session *const restrict p)
{
	/* This is complicated.
	 *
	 * Clients are restricted to sending us 300 bytes (400 Base-64 characters), but the mechanism
	 * that they have chosen could require them to send more than this amount, so they have to send
	 * it 400 Base-64 characters at a time in stages. When we receive data less than 400 characters,
	 * we know we don't need to buffer any more data, and can finally process it.
	 *
	 * However, if the client wants to send us a multiple of 400 characters and no more, we would be
	 * waiting forever for them to send 'the rest', even though there isn't any. This is solved by
	 * having them send a single '+' character to indicate that they have no more data to send.
	 *
	 * This is also what clients send us when they do not want to send us any data at all, and in
	 * either event, this is *NOT* *DATA* we are receiving, and we should not buffer it.
	 *
	 * Also, if the data is a single '*' character, the client is aborting authentication. Servers
	 * should send us a 'D' packet instead of a 'C *' packet in this case, but this is for if they
	 * don't. Note that this will usually result in the client getting a 904 numeric instead of 906,
	 * but the alternative is not treating '*' specially and then going on to fail to decode it in
	 * sasl_process_input() above, which will result in ... an aborted session and a 904 numeric.
	 * So this just saves time.
	 */

	const size_t len = strlen(smsg->parv[0]);

	// Abort?
	if (len == 1 && smsg->parv[0][0] == '*')
		return false;

	// End of data?
	if (len == 1 && smsg->parv[0][0] == '+')
	{
		if (p->buf)
			return sasl_process_buffer(p);

		// This function already deals with the special case of 1 '+' character
		return sasl_process_packet(p, smsg->parv[0], len);
	}

	/* Optimisation: If there is no buffer yet and this data is less than 400 characters, we don't
	 * need to buffer it at all, and can process it immediately.
	 */
	if (! p->buf && len < SASL_S2S_MAXLEN_ATONCE_B64)
		return sasl_process_packet(p, smsg->parv[0], len);

	/* We need to buffer the data now, but first check if the client hasn't sent us an excessive
	 * amount already.
	 */
	if ((p->len + len) > SASL_S2S_MAXLEN_TOTAL_B64)
	{
		(void) slog(LG_DEBUG, "%s: client %s has exceeded allowed data length", MOWGLI_FUNC_NAME, p->uid);
		return false;
	}

	// (Re)allocate a buffer, append the received data to it, and update its recorded length.
	p->buf = srealloc(p->buf, p->len + len + 1);
	(void) memcpy(p->buf + p->len, smsg->parv[0], len);
	p->len += len;

	// Messages not exactly 400 characters are the end of data.
	if (len < SASL_S2S_MAXLEN_ATONCE_B64)
		return sasl_process_buffer(p);

	return true;
}

static void
sasl_input(struct sasl_message *const restrict smsg)
{
	struct sasl_session *const p = sasl_session_find_or_make(smsg);

	bool ret = true;

	switch (smsg->mode)
	{
		case 'H':
			// (H)ost information
			(void) sasl_input_hostinfo(smsg, p);
			break;

		case 'S':
			// (S)tart authentication
			ret = sasl_input_startauth(smsg, p);
			break;

		case 'C':
			// (C)lient data
			ret = sasl_input_clientdata(smsg, p);
			break;

		case 'D':
			// (D)one -- when we receive it, means client abort
			(void) sasl_session_reset_or_destroy(p);
			break;
	}

	if (! ret)
		(void) sasl_session_abort(p);
}

static void
sasl_user_add(struct hook_user_nick *const restrict data)
{
	// If the user has been killed, don't do anything.
	struct user *const u = data->u;
	if (! u)
		return;

	// Not concerned unless it's an SASL login.
	struct sasl_session *const p = sasl_session_find(u->uid);
	if (! p)
		return;

	(void) sasl_handle_login(p, u, NULL);
	(void) sasl_session_destroy(p);
}

static void
sasl_delete_stale(void ATHEME_VATTR_UNUSED *const restrict vptr)
{
	mowgli_node_t *n, *tn;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, sasl_sessions.head)
	{
		struct sasl_session *const p = n->data;

		if (p->flags & ASASL_SFLAG_MARKED_FOR_DELETION)
			(void) sasl_session_destroy(p);
		else
			p->flags |= ASASL_SFLAG_MARKED_FOR_DELETION;
	}
}

static void
sasl_mech_register(const struct sasl_mechanism *const restrict mech)
{
	if (sasl_mechanism_find(mech->name))
	{
		(void) slog(LG_DEBUG, "%s: ignoring attempt to register %s again", MOWGLI_FUNC_NAME, mech->name);
		return;
	}

	(void) slog(LG_DEBUG, "%s: registering %s", MOWGLI_FUNC_NAME, mech->name);

	mowgli_node_t *const node = mowgli_node_create();

	if (! node)
	{
		(void) slog(LG_ERROR, "%s: mowgli_node_create() failed; out of memory?", MOWGLI_FUNC_NAME);
		return;
	}

	/* Here we cast it to (void *) because mowgli_node_add() expects that; it cannot be made const because then
	 * it would have to return a (const void *) too which would cause multiple warnings any time it is actually
	 * storing, and thus gets assigned to, a pointer to a mutable object.
	 *
	 * To avoid the cast generating a diagnostic due to dropping a const qualifier, we first cast to uintptr_t.
	 * This is not unprecedented in this codebase; libathemecore/crypto.c & libathemecore/strshare.c do the
	 * same thing.
	 */
	(void) mowgli_node_add((void *)((uintptr_t) mech), node, &sasl_mechanisms);

	(void) sasl_mechlist_do_rebuild();
}

static void
sasl_mech_unregister(const struct sasl_mechanism *const restrict mech)
{
	mowgli_node_t *n, *tn;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, sasl_sessions.head)
	{
		struct sasl_session *const session = n->data;

		if (session->mechptr == mech)
		{
			(void) slog(LG_DEBUG, "%s: destroying session %s", MOWGLI_FUNC_NAME, session->uid);
			(void) sasl_session_destroy(session);
		}
	}
	MOWGLI_ITER_FOREACH_SAFE(n, tn, sasl_mechanisms.head)
	{
		if (n->data == mech)
		{
			(void) slog(LG_DEBUG, "%s: unregistering %s", MOWGLI_FUNC_NAME, mech->name);
			(void) mowgli_node_delete(n, &sasl_mechanisms);
			(void) mowgli_node_free(n);
			(void) sasl_mechlist_do_rebuild();

			break;
		}
	}
}

static inline bool ATHEME_FATTR_WUR
sasl_authxid_can_login(struct sasl_session *const restrict p, const char *const restrict authxid,
                       struct myuser **const restrict muo, char *const restrict val_name,
                       char *const restrict val_eid, const char *const restrict other_val_eid)
{
	return_val_if_fail(p != NULL, false);
	return_val_if_fail(p->si != NULL, false);
	return_val_if_fail(p->mechptr != NULL, false);

	struct myuser *const mu = myuser_find_by_nick(authxid);

	if (! mu)
	{
		(void) slog(LG_DEBUG, "%s: myuser_find_by_nick: does not exist", MOWGLI_FUNC_NAME);
		return false;
	}
	if (metadata_find(mu, "private:freeze:freezer"))
	{
		(void) logcommand(p->si, CMDLOG_LOGIN, "failed LOGIN to \2%s\2 (frozen)", entity(mu)->name);
		return false;
	}

	if (muo)
		*muo = mu;

	(void) mowgli_strlcpy(val_name, entity(mu)->name, NICKLEN + 1);
	(void) mowgli_strlcpy(val_eid, entity(mu)->id, IDLEN + 1);

	if (p->mechptr->password_based && (mu->flags & MU_NOPASSWORD))
	{
		(void) logcommand(p->si, CMDLOG_LOGIN, "failed LOGIN %s to \2%s\2 (password authentication disabled)",
		                  p->mechptr->name, entity(mu)->name);

		return false;
	}

	if (strcmp(val_eid, other_val_eid) == 0)
		// We have already executed the user_can_login hook for this user
		return true;

	struct hook_user_login_check req = {

		.si         = p->si,
		.mu         = mu,
		.allowed    = true,
	};

	(void) hook_call_user_can_login(&req);

	if (! req.allowed)
		(void) logcommand(p->si, CMDLOG_LOGIN, "failed LOGIN to \2%s\2 (denied by hook)", entity(mu)->name);

	return req.allowed;
}

static bool ATHEME_FATTR_WUR
sasl_authcid_can_login(struct sasl_session *const restrict p, const char *const restrict authcid,
                       struct myuser **const restrict muo)
{
	return sasl_authxid_can_login(p, authcid, muo, p->authcid, p->authceid, p->authzeid);
}

static bool ATHEME_FATTR_WUR
sasl_authzid_can_login(struct sasl_session *const restrict p, const char *const restrict authzid,
                       struct myuser **const restrict muo)
{
	return sasl_authxid_can_login(p, authzid, muo, p->authzid, p->authzeid, p->authceid);
}

extern const struct sasl_core_functions sasl_core_functions;
const struct sasl_core_functions sasl_core_functions = {

	.mech_register      = &sasl_mech_register,
	.mech_unregister    = &sasl_mech_unregister,
	.authcid_can_login  = &sasl_authcid_can_login,
	.authzid_can_login  = &sasl_authzid_can_login,
	.recalc_mechlist    = &sasl_mechlist_string_build,
};

static void
saslserv_message_handler(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	// this should never happen
	if (parv[0][0] == '&')
	{
		(void) slog(LG_ERROR, "%s: got parv with local channel: %s", MOWGLI_FUNC_NAME, parv[0]);
		return;
	}

	// make a copy of the original for debugging
	char orig[BUFSIZE];
	(void) mowgli_strlcpy(orig, parv[parc - 1], sizeof orig);

	// lets go through this to get the command
	char *const cmd = strtok(parv[parc - 1], " ");
	char *const text = strtok(NULL, "");

	if (! cmd)
		return;

	if (*orig == '\001')
	{
		(void) handle_ctcp_common(si, cmd, text);
		return;
	}

	(void) command_fail(si, fault_noprivs, _("This service exists to identify connecting clients "
	                                         "to the network. It has no public interface."));
}

static void
mod_init(struct module *const restrict m)
{
	if (! (saslsvs = service_add("saslserv", &saslserv_message_handler)))
	{
		(void) slog(LG_ERROR, "%s: service_add() failed", m->name);
		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) hook_add_sasl_input(&sasl_input);
	(void) hook_add_user_add(&sasl_user_add);
	(void) hook_add_server_eob(&sasl_server_eob);

	sasl_delete_stale_timer = mowgli_timer_add(base_eventloop, "sasl_delete_stale", &sasl_delete_stale, NULL, SECONDS_PER_MINUTE / 2);
	authservice_loaded++;

	(void) add_bool_conf_item("HIDE_SERVER_NAMES", &saslsvs->conf_table, 0, &sasl_hide_server_names, false);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) hook_del_sasl_input(&sasl_input);
	(void) hook_del_user_add(&sasl_user_add);
	(void) hook_del_server_eob(&sasl_server_eob);

	(void) mowgli_timer_destroy(base_eventloop, sasl_delete_stale_timer);

	(void) del_conf_item("HIDE_SERVER_NAMES", &saslsvs->conf_table);
	(void) service_delete(saslsvs);

	authservice_loaded--;

	if (sasl_sessions.head)
		(void) slog(LG_ERROR, "saslserv/main: shutting down with a non-empty session list; "
		                      "a mechanism did not unregister itself! (BUG)");
}

SIMPLE_DECLARE_MODULE_V1("saslserv/main", MODULE_UNLOAD_CAPABILITY_OK)
