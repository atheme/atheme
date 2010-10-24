/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"saslserv/main", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

mowgli_list_t saslserv_conftable;

mowgli_list_t sessions;
mowgli_list_t sasl_mechanisms;

sasl_session_t *find_session(char *uid);
sasl_session_t *make_session(char *uid);
void destroy_session(sasl_session_t *p);
static void sasl_logcommand(sasl_session_t *p, myuser_t *login, int level, const char *fmt, ...);
static void sasl_input(sasl_message_t *smsg);
static void sasl_packet(sasl_session_t *p, char *buf, int len);
static void sasl_write(char *target, char *data, int length);
int login_user(sasl_session_t *p);
static void sasl_newuser(hook_user_nick_t *data);
static void delete_stale(void *vptr);

/* main services client routine */
static void saslserv(sourceinfo_t *si, int parc, char *parv[])
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
	
	command_fail(si, fault_noprivs, "This service exists to identify "
			"connecting clients to the network. It has no "
			"public interface.");
}

service_t *saslsvs = NULL;

void _modinit(module_t *m)
{
	hook_add_event("sasl_input");
	hook_add_sasl_input(sasl_input);
	hook_add_event("user_add");
	hook_add_user_add(sasl_newuser);
	event_add("sasl_delete_stale", delete_stale, NULL, 30);

	saslsvs = service_add("saslserv", saslserv, &saslserv_conftable);
	authservice_loaded++;
}

void _moddeinit(void)
{
	mowgli_node_t *n, *tn;

	hook_del_sasl_input(sasl_input);
	hook_del_user_add(sasl_newuser);
	event_delete(delete_stale, NULL);

        if (saslsvs != NULL)
		service_delete(saslsvs);

	authservice_loaded--;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, sessions.head)
	{
		destroy_session(n->data);
		mowgli_node_delete(n, &sessions);
		mowgli_node_free(n);
	}
}

/*
 * Begin SASL-specific code
 */

/* find an existing session by uid */
sasl_session_t *find_session(char *uid)
{
	sasl_session_t *p;
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, sessions.head)
	{
		p = n->data;
		if(!strcmp(p->uid, uid))
			return p;
	}

	return NULL;
}

/* create a new session if it does not already exist */
sasl_session_t *make_session(char *uid)
{
	sasl_session_t *p = find_session(uid);
	mowgli_node_t *n;

	if(p)
		return p;

	p = malloc(sizeof(sasl_session_t));
	memset(p, 0, sizeof(sasl_session_t));
	strlcpy(p->uid, uid, IDLEN);

	n = mowgli_node_create();
	mowgli_node_add(p, n, &sessions);

	return p;
}

/* free a session and all its contents */
void destroy_session(sasl_session_t *p)
{
	mowgli_node_t *n, *tn;
	myuser_t *mu;

	if (p->flags & ASASL_NEED_LOG && p->username != NULL)
	{
		mu = myuser_find(p->username);
		if (mu != NULL)
			sasl_logcommand(p, mu, CMDLOG_LOGIN, "LOGIN (session timed out)");
	}

	MOWGLI_ITER_FOREACH_SAFE(n, tn, sessions.head)
	{
		if(n->data == p)
		{
			mowgli_node_delete(n, &sessions);
			mowgli_node_free(n);
		}
	}

	free(p->buf);
	p->buf = p->p = NULL;
	if(p->mechptr)
		p->mechptr->mech_finish(p); /* Free up any mechanism data */
	p->mechptr = NULL; /* We're not freeing the mechanism, just "dereferencing" it */
	free(p->username);

	free(p);
}

/* interpret an AUTHENTICATE message */
static void sasl_input(sasl_message_t *smsg)
{
	sasl_session_t *p = make_session(smsg->uid);
	int len = strlen(smsg->buf);

	/* Abort packets, or maybe some other kind of (D)one */
	if(smsg->mode == 'D')
	{
		destroy_session(p);
		return;
	}

	if(smsg->mode != 'S' && smsg->mode != 'C')
		return;

	if(p->buf == NULL)
	{
		p->buf = (char *)malloc(len + 1);
		p->p = p->buf;
		p->len = len;
	}
	else
	{
		if(p->len + len + 1 > 8192) /* This is a little much... */
		{
			sasl_sts(p->uid, 'D', "F");
			destroy_session(p);
			return;
		}

		p->buf = (char *)realloc(p->buf, p->len + len + 1);
		p->p = p->buf + p->len;
		p->len += len;
	}

	memcpy(p->p, smsg->buf, len);

	/* Messages not exactly 400 bytes are the end of a packet. */
	if(len < 400)
	{
		p->buf[p->len] = '\0';
		sasl_packet(p, p->buf, p->len);
		free(p->buf);
		p->buf = p->p = NULL;
		p->len = 0;
	}
}

/* find a mechanism by name */
static sasl_mechanism_t *find_mechanism(char *name)
{
	mowgli_node_t *n;
	sasl_mechanism_t *mptr;

	MOWGLI_ITER_FOREACH(n, sasl_mechanisms.head)
	{
		mptr = n->data;
		if(!strcmp(mptr->name, name))
			return mptr;
	}

	slog(LG_DEBUG, "find_mechanism(): cannot find mechanism `%s'!", name);

	return NULL;
}

/* given an entire sasl message, advance session by passing data to mechanism
 * and feeding returned data back to client.
 */
static void sasl_packet(sasl_session_t *p, char *buf, int len)
{
	int rc;
	size_t tlen = 0;
	char *cloak, *out = NULL;
	char *temp;
	char mech[21];
	int out_len = 0;
	metadata_t *md;

	/* First piece of data in a session is the name of
	 * the SASL mechanism that will be used.
	 */
	if(!p->mechptr)
	{
		if(len > 20)
		{
			sasl_sts(p->uid, 'D', "F");
			destroy_session(p);
			return;
		}

		memcpy(mech, buf, len);
		mech[len] = '\0';

		if(!(p->mechptr = find_mechanism(mech)))
		{
			/* Generate a list of supported mechanisms (disabled since charybdis doesn't support this yet). */
/*			
			char temp[400], *ptr = temp;
			int l = 0;
			mowgli_node_t *n;

			MOWGLI_ITER_FOREACH(n, sasl_mechanisms.head)
			{
				sasl_mechanism_t *mptr = n->data;
				if(l + strlen(mptr->name) > 510)
					break;
				strcpy(ptr, mptr->name);
				ptr += strlen(mptr->name);
				*ptr++ = ',';
				l += strlen(mptr->name) + 1;
			}

			if(l)
				ptr--;
			*ptr = '\0';

			sasl_sts(p->uid, 'M', temp);
			*/

			sasl_sts(p->uid, 'D', "F");
			destroy_session(p);
			return;
		}

		rc = p->mechptr->mech_start(p, &out, &out_len);
	}else{
		if(base64_decode_alloc(buf, len, &temp, &tlen))
		{
			rc = p->mechptr->mech_step(p, temp, tlen, &out, &out_len);
			free(temp);
		}else
			rc = ASASL_FAIL;
	}

	/* Some progress has been made, reset timeout. */
	p->flags &= ~ASASL_MARKED_FOR_DELETION;

	if(rc == ASASL_DONE)
	{
		myuser_t *mu = myuser_find(p->username);
		if(mu && login_user(p))
		{
			if ((md = metadata_find(mu, "private:usercloak")))
				cloak = md->value;
			else
				cloak = "*";

			if (!(mu->flags & MU_WAITAUTH))
				svslogin_sts(p->uid, "*", "*", cloak, entity(mu)->name);
			sasl_sts(p->uid, 'D', "S");
		}
		else
			sasl_sts(p->uid, 'D', "F");
		/* Will destroy session on introduction of user to net. */
		return;
	}
	else if(rc == ASASL_MORE)
	{
		if(out_len)
		{
			if(base64_encode_alloc(out, out_len, &temp))
			{
				sasl_write(p->uid, temp, strlen(temp));
				free(temp);
				free(out);
				return;
			}
		}
		else
		{
			sasl_sts(p->uid, 'C', "+");
			free(out);
			return;
		}
	}

	free(out);
	sasl_sts(p->uid, 'D', "F");
	destroy_session(p);
}

/* output an arbitrary amount of data to the SASL client */
static void sasl_write(char *target, char *data, int length)
{
	char out[401];
	int last = 400, rem = length;

	while(rem)
	{
		int nbytes = rem > 400 ? 400 : rem;
		memcpy(out, data, nbytes);
		out[nbytes] = '\0';
		sasl_sts(target, 'C', out);

		data += nbytes;
		rem -= nbytes;
		last = nbytes;
	}
	
	/* The end of a packet is indicated by a string not of length 400.
	 * If last piece is exactly 400 in size, send an empty string to
	 * finish the transaction.
	 * Also if there is no data at all.
	 */
	if(last == 400)
		sasl_sts(target, 'C', "+");
}

static void sasl_logcommand(sasl_session_t *p, myuser_t *mu, int level, const char *fmt, ...)
{
	va_list args;
	char lbuf[BUFSIZE];
	
	va_start(args, fmt);
	vsnprintf(lbuf, BUFSIZE, fmt, args);
	slog(level, "%s %s:%s %s", saslsvs->internal_name, mu ? entity(mu)->name : "", p->uid, lbuf);
	va_end(args);
}

/* authenticated, now double check that their account is ok for login */
int login_user(sasl_session_t *p)
{
	myuser_t *mu = myuser_find(p->username);

	if(mu == NULL) /* WTF? */
		return 0;

 	if (metadata_find(mu, "private:freeze:freezer"))
	{
		sasl_logcommand(p, NULL, CMDLOG_LOGIN, "failed LOGIN to \2%s\2 (frozen)", entity(mu)->name);
		return 0;
	}

	if (MOWGLI_LIST_LENGTH(&mu->logins) >= me.maxlogins)
	{
		sasl_logcommand(p, NULL, CMDLOG_LOGIN, "failed LOGIN to \2%s\2 (too many logins)", entity(mu)->name);
		return 0;
	}

	/* Log it with the full n!u@h later */
	p->flags |= ASASL_NEED_LOG;

	return 1;
}

/* clean up after a user who is finally on the net */
static void sasl_newuser(hook_user_nick_t *data)
{
	user_t *u = data->u;
	sasl_session_t *p;
	myuser_t *mu;

	/* If the user has been killed, don't do anything. */
	if (!u)
		return;

	p = find_session(u->uid);

	/* Not concerned unless it's a SASL login. */
	if(p == NULL)
		return;

	/* We will log it ourselves, if needed */
	p->flags &= ~ASASL_NEED_LOG;

	/* Find the account */
	mu = p->username ? myuser_find(p->username) : NULL;
	if (mu == NULL)
	{
		notice(saslsvs->nick, u->nick, "Account %s dropped, login cancelled",
				p->username ? p->username : "??");
		destroy_session(p);
		/* We'll remove their ircd login in handle_burstlogin() */
		return;
	}
	destroy_session(p);

	myuser_login(saslsvs, u, mu, false);

	logcommand_user(saslsvs, u, CMDLOG_LOGIN, "LOGIN");
}

/* This function is run approximately once every 30 seconds.
 * It looks for flagged sessions, and deletes them, while
 * flagging all the others. This way stale sessions are deleted
 * after no more than 60 seconds.
 */
static void delete_stale(void *vptr)
{
	sasl_session_t *p;
	mowgli_node_t *n, *tn;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, sessions.head)
	{
		p = n->data;
		if(p->flags & ASASL_MARKED_FOR_DELETION)
		{
			mowgli_node_delete(n, &sessions);
			destroy_session(p);
			mowgli_node_free(n);
		} else
			p->flags |= ASASL_MARKED_FOR_DELETION;
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
