/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 * $Id: main.c 5718 2006-07-04 06:10:05Z gxti $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"saslserv/main", FALSE, _modinit, _moddeinit,
	"$Id: main.c 5718 2006-07-04 06:10:05Z gxti $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t sessions;
list_t sasl_mechanisms;

sasl_session_t *find_session(char *uid);
sasl_session_t *make_session(char *uid);
void destroy_session(sasl_session_t *p);
static void sasl_input(void *vptr);
static void sasl_packet(sasl_session_t *p, char *buf, int len);
static void sasl_write(char *target, char *data, int length);
int login_user(sasl_session_t *p);
static void user_burstlogin(void *vptr);
static void delete_stale(void *vptr);

/* main services client routine */
static void saslserv(char *origin, uint8_t parc, char *parv[])
{
	char *cmd, *s;
	char orig[BUFSIZE];

	if (!origin)
	{
		slog(LG_DEBUG, "services(): recieved a request with no origin!");
		return;
	}

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

	if (!cmd)
		return;

	/* ctcp? case-sensitive as per rfc */
	if (!strcmp(cmd, "\001PING"))
	{
		if (!(s = strtok(NULL, " ")))
			s = " 0 ";

		strip(s);
		notice(saslsvs.nick, origin, "\001PING %s\001", s);
		return;
	}
	else if (!strcmp(cmd, "\001VERSION\001"))
	{
		notice(saslsvs.nick, origin,
		       "\001VERSION atheme-%s. %s %s %s%s%s%s%s%s%s%s%s TS5ow\001",
		       version, revision, me.name,
		       (match_mapping) ? "A" : "",
		       (me.loglevel & LG_DEBUG) ? "d" : "",
		       (me.auth) ? "e" : "",
		       (config_options.flood_msgs) ? "F" : "",
		       (config_options.leave_chans) ? "l" : "", (config_options.join_chans) ? "j" : "", (!match_mapping) ? "R" : "", (config_options.raw) ? "r" : "", (runflags & RF_LIVE) ? "n" : "");

		return;
	}
	else if (!strcmp(cmd, "\001CLIENTINFO\001"))
	{
		/* easter eggs are mandatory these days */
		notice(saslsvs.nick, origin, "\001CLIENTINFO LILITH\001");
		return;
	}

	/* ctcps we don't care about are ignored */
	else if (*cmd == '\001')
		return;

	notice(saslsvs.nick, origin, "This service exists to identify connecting clients to the network. It has no public interface.");
}

static void saslserv_config_ready(void *unused)
{
        if (saslsvs.me)
                del_service(saslsvs.me);

        saslsvs.me = add_service(saslsvs.nick, saslsvs.user,
                                 saslsvs.host, saslsvs.real, saslserv);

        hook_del_hook("config_ready", saslserv_config_ready);
}

void _modinit(module_t *m)
{
	hook_add_event("config_ready");
	hook_add_hook("config_ready", saslserv_config_ready);
	hook_add_event("sasl_input");
	hook_add_hook("sasl_input", sasl_input);
	hook_add_event("user_burstlogin");
	hook_add_hook("user_burstlogin", user_burstlogin);
	event_add("sasl_delete_stale", delete_stale, NULL, 15);

        if (!cold_start)
        {
                saslsvs.me = add_service(saslsvs.nick, saslsvs.user,
			saslsvs.host, saslsvs.real, saslserv);
        }
	authservice_loaded++;
}

void _moddeinit(void)
{
	node_t *n, *tn;

	hook_del_hook("sasl_input", sasl_input);
	hook_del_hook("user_burstlogin", user_burstlogin);
	event_delete(delete_stale, NULL);

        if (saslsvs.me)
	{
		del_service(saslsvs.me);
		saslsvs.me = NULL;
	}
	authservice_loaded--;

	LIST_FOREACH_SAFE(n, tn, sessions.head)
	{
		destroy_session(n->data);
		node_del(n, &sessions);
		node_free(n);
	}

	LIST_FOREACH_SAFE(n, tn, saslsvs.pending.head)
	{
		free(n->data);
		node_del(n, &saslsvs.pending);
		node_free(n);
	}
}

/*
 * Begin SASL-specific code
 */

/* find an existing session by uid */
sasl_session_t *find_session(char *uid)
{
	sasl_session_t *p;
	node_t *n;

	LIST_FOREACH(n, sessions.head)
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
	node_t *n;

	if(p)
		return p;

	p = malloc(sizeof(sasl_session_t));
	memset(p, 0, sizeof(sasl_session_t));
	strlcpy(p->uid, uid, NICKLEN);

	n = node_create();
	node_add(p, n, &sessions);

	return p;
}

/* free a session and all its contents */
void destroy_session(sasl_session_t *p)
{
	node_t *n, *tn;
	int i;

	LIST_FOREACH_SAFE(n, tn, sessions.head)
	{
		if(n->data == p)
		{
			node_del(n, &sessions);
			node_free(n);
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
static void sasl_input(void *vptr)
{
	sasl_message_t *msg = vptr;
	sasl_session_t *p = make_session(msg->uid);
	int len = strlen(msg->buf);

	/* Abort packets, or maybe some other kind of (D)one */
	if(msg->mode == 'D')
	{
		destroy_session(p);
		return;
	}

	if(msg->mode != 'S' && msg->mode != 'C')
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

	memcpy(p->p, msg->buf, len);

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
	node_t *n;
	sasl_mechanism_t *mptr;

	LIST_FOREACH(n, sasl_mechanisms.head)
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
	int rc, i;
	size_t tlen = 0;
	char *cloak, *out = NULL;
	char *temp;
	char mech[21];
	int out_len = 0;
	metadata_t *md;
	myuser_t *mu;
	node_t *n;

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

			LIST_FOREACH(n, sasl_mechanisms.head)
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

	if(rc == ASASL_DONE)
	{
		myuser_t *mu = myuser_find(p->username);
		if(mu && login_user(p))
		{
			mu->flags |= MU_SASL;

			if ((md = metadata_find(mu, METADATA_USER, "private:usercloak")))
				cloak = md->value;
			else
				cloak = "*";

			svslogin_sts(p->uid, "*", "*", cloak, mu->name);
			sasl_sts(p->uid, 'D', "S");

            n = node_create();
            node_add(strdup(p->uid), n, &saslsvs.pending);
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
	int last, rem = length;

	while(rem)
	{
		int send = rem > 400 ? 400 : rem;
		memcpy(out, data, send);
		out[send] = '\0';
		sasl_sts(target, 'C', out);

		data += send;
		rem -= send;
		last = send;
	}

	/* The end of a packet is indicated by a string not of length 400.
	 * If last piece is exactly 400 in size, send an empty string to
	 * finish the transaction.
	 */
	if(last == 400)
		sasl_sts(target, 'C', "+");
}

void sasl_logcommand(char *source, int level, const char *fmt, ...)
{
	va_list args;
	time_t t;
	struct tm tm;
	char datetime[64];
	char lbuf[BUFSIZE], strfbuf[32];

	/* XXX use level */

	va_start(args, fmt);

	time(&t);
	tm = *localtime(&t);
	strftime(datetime, sizeof(datetime) - 1, "[%d/%m/%Y %H:%M:%S]", &tm);

	vsnprintf(lbuf, BUFSIZE, fmt, args);

	if (!log_file)
		log_open();

	if (log_file)
	{
		fprintf(log_file, "%s sasl_agent %s %s\n",
				datetime, source, lbuf);

		fflush(log_file);
	}

	va_end(args);
}

/* authenticated, now double check that their account is ok for login */
int login_user(sasl_session_t *p)
{
	myuser_t *mu = myuser_find(p->username);
	metadata_t *md;
	char *target = p->uid;

	if(mu == NULL) /* WTF? */
		return 0;

 	if (md = metadata_find(mu, METADATA_USER, "private:freeze:freezer"))
	{
		sasl_logcommand(target, CMDLOG_LOGIN, "failed IDENTIFY to %s (frozen)", mu->name);
		return 0;
	}

	if (LIST_LENGTH(&mu->logins) >= me.maxlogins)
	{
		sasl_logcommand(target, CMDLOG_LOGIN, "failed IDENTIFY to %s (too many logins)", mu->name);
		return 0;
	}

	sasl_logcommand(target, CMDLOG_LOGIN, "IDENTIFY");

	return 1;
}

/* clean up after a user who is finally on the net */
static void user_burstlogin(void *vptr)
{
	user_t *u = vptr;
	sasl_session_t *p = find_session(u->uid);
	metadata_t *md_failnum;
	char lau[BUFSIZE], lao[BUFSIZE];
	char strfbuf[BUFSIZE];
	struct tm tm;
	myuser_t *mu;
	node_t *n, *tn;

	/* Remove any pending login marker. */
	LIST_FOREACH_SAFE(n, tn, saslsvs.pending.head)
	{
		if(!strcmp((char*)n->data, u->uid))
		{
			node_del(n, &saslsvs.pending);
			free(n->data);
			node_free(n);
		}
	}

	/* Not concerned unless it's a SASL login. */
	if(p == NULL)
		return;
	destroy_session(p);

	/* WTF? */
	if((mu = u->myuser) == NULL)
		return;

	if (is_soper(mu))
	{
		snoop("SOPER: \2%s\2 as \2%s\2", u->nick, mu->name);
	}

	myuser_notice(saslsvs.nick, mu, "%s!%s@%s has just authenticated as you (%s)", u->nick, u->user, u->vhost, mu->name);

	u->myuser = mu;
	n = node_create();
	node_add(u, n, &mu->logins);

	/* keep track of login address for users */
	strlcpy(lau, u->user, BUFSIZE);
	strlcat(lau, "@", BUFSIZE);
	strlcat(lau, u->vhost, BUFSIZE);
	metadata_add(mu, METADATA_USER, "private:host:vhost", lau);

	/* and for opers */
	strlcpy(lao, u->user, BUFSIZE);
	strlcat(lao, "@", BUFSIZE);
	strlcat(lao, u->host, BUFSIZE);
	metadata_add(mu, METADATA_USER, "private:host:actual", lao);

	/* check for failed attempts and let them know */
	if ((md_failnum = metadata_find(mu, METADATA_USER, "private:loginfail:failnum")) && (atoi(md_failnum->value) > 0))
	{
		metadata_t *md_failtime, *md_failaddr;
		time_t ts;

		tm = *localtime(&mu->lastlogin);
		strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

		notice(saslsvs.nick, u->nick, "\2%d\2 failed %s since %s.",
			atoi(md_failnum->value), (atoi(md_failnum->value) == 1) ? "login" : "logins", strfbuf);

		md_failtime = metadata_find(mu, METADATA_USER, "private:loginfail:lastfailtime");
		ts = atol(md_failtime->value);
		md_failaddr = metadata_find(mu, METADATA_USER, "private:loginfail:lastfailaddr");

		tm = *localtime(&ts);
		strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

		notice(saslsvs.nick, u->nick, "Last failed attempt from: \2%s\2 on %s.",
			md_failaddr->value, strfbuf);

		metadata_delete(mu, METADATA_USER, "private:loginfail:failnum");	/* md_failnum now invalid */
		metadata_delete(mu, METADATA_USER, "private:loginfail:lastfailtime");
		metadata_delete(mu, METADATA_USER, "private:loginfail:lastfailaddr");
	}

	mu->lastlogin = CURRTIME;
	hook_call_event("user_identify", u);
}

/* This function is run approximately once every 15 seconds.
 * It looks for flagged sessions, and deletes them, while
 * flagging all the others. This way stale sessions are deleted
 * after no more than 30 seconds.
 */
static void delete_stale(void *vptr)
{
	sasl_session_t *p;
	node_t *n, *tn, *m, *tm;

	LIST_FOREACH_SAFE(n, tn, sessions.head)
	{
		p = n->data;
		if(p->flags & ASASL_MARKED_FOR_DELETION)
		{
			/* Remove any pending login marker. */
			LIST_FOREACH_SAFE(m, tm, saslsvs.pending.head)
			{
				if(!strcmp((char*)m->data, p->uid))
				{
					node_del(m, &saslsvs.pending);
					free(m->data);
					node_free(m);
				}
			}

			node_del(n, &sessions);
			destroy_session(p);
			node_free(n);
		}

		p->flags |= ASASL_MARKED_FOR_DELETION;
	}
}

/* vim: set ts=8 sts=8 noexpandtab: */
