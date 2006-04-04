/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Provides a SASL authentication agent for clients.
 *
 * $Id: sasl.c 4859 2006-02-20 06:13:53Z gxti $
 */

/* sasl.h and friends are included from atheme.h now --nenolod */
#include "atheme.h"

DECLARE_MODULE_V1
(
	"saslserv/sasl", FALSE, _modinit, _moddeinit,
	"$Id: sasl.c 4859 2006-02-20 06:13:53Z gxti $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t sessions[HASHSIZE];
static BlockHeap *session_heap;
Gsasl *ctx = NULL;

sasl_session_t *find_session(char *uid);
sasl_session_t *make_session(char *uid);
void destroy_session(sasl_session_t *p);
static void sasl_input(void *vptr);
static void sasl_packet(sasl_session_t *p, char *buf, int len);
static void sasl_write(char *target, char *data);
int login_user(sasl_session_t *p, char *authid);
int callback(Gsasl *ctx, Gsasl_session *sctx, Gsasl_property prop);
static void user_burstlogin(void *vptr);

void _modinit(module_t *m)
{
	session_heap = BlockHeapCreate(sizeof(sasl_session_t), 64);

	hook_add_event("sasl_input");
	hook_add_hook("sasl_input", sasl_input);
	hook_add_event("user_burstlogin");
	hook_add_hook("user_burstlogin", user_burstlogin);

	/* XXX: Check for failure. */
	gsasl_init(&ctx);
	gsasl_callback_set(ctx, callback);
}

void _moddeinit()
{
	int i;
	node_t *n;
	for(i = 0;i < HASHSIZE;i++)
		LIST_FOREACH(n, sessions[i].head)
			destroy_session(n->data);

	BlockHeapDestroy(session_heap);
	hook_del_hook("sasl_input", sasl_input);
	hook_del_hook("user_burstlogin", user_burstlogin);

	gsasl_done(ctx);
	ctx = NULL;
}

sasl_session_t *find_session(char *uid)
{
	sasl_session_t *p;
	node_t *n;

	LIST_FOREACH(n, sessions[SHASH((unsigned char *) uid)].head)
	{
		p = n->data;
		if(!strcmp(p->uid, uid))
			return p;
	}

	return NULL;
}

sasl_session_t *make_session(char *uid)
{
	sasl_session_t *p = find_session(uid);
	node_t *n;

	if(p)
		return p;

	p = BlockHeapAlloc(session_heap);
	strlcpy(p->uid, uid, NICKLEN);

	n = node_create();
	node_add(p, n, &sessions[SHASH((unsigned char *) uid)]);

	return p;
}

void destroy_session(sasl_session_t *p)
{
	node_t *n;
	int i;

	LIST_FOREACH(n, sessions[SHASH((unsigned char *) p->uid)].head)
	{
		if(n->data == p)
		{
			node_del(n, &sessions[SHASH((unsigned char *) p->uid)]);
			break;
		}
	}

	free(p->buf);
	p->buf = p->p = NULL;
	p->sctx = NULL;

	BlockHeapFree(session_heap, p);
}

int callback(Gsasl *ctx, Gsasl_session *sctx, Gsasl_property prop)
{
	myuser_t *mu = myuser_find((char*)gsasl_property_fast(sctx, GSASL_AUTHID));

	if(!mu)
		return GSASL_NO_CALLBACK;

	switch(prop)
	{
		case GSASL_VALIDATE_SIMPLE:
			return verify_password(mu, (char*)gsasl_property_fast(sctx, GSASL_PASSWORD)) ? GSASL_OK : GSASL_AUTHENTICATION_ERROR;

		case GSASL_PASSWORD:
			if(mu->flags & MU_CRYPTPASS) /* Crap! Perhaps filter mechs that this user doesn't support? */
				return GSASL_NO_CALLBACK;
			gsasl_property_set(sctx, GSASL_PASSWORD, mu->pass);
			return GSASL_OK;

		default:
			return GSASL_NO_CALLBACK;
	}
}

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

static void sasl_packet(sasl_session_t *p, char *buf, int len)
{
	char *out;
	int rc, i;
	char *pt = buf;
	int ol = len;
	char *cloak;
	metadata_t *md;
	myuser_t *mu;

	/* First piece of data in a session is the name of
	 * the SASL mechanism that will be used.
	 */
	if(!*p->mech)
	{
		if(len > 20)
		{
			sasl_sts(p->uid, 'D', "F");
			destroy_session(p);
			return;
		}

		strlcpy(p->mech, buf, 21);

		if(gsasl_server_start(ctx, p->mech, &p->sctx) != GSASL_OK)
		{
			char *buf, *mechs;
			int l;

			/* Generate a list of supported mechanisms. */
			gsasl_server_mechlist(ctx, &mechs);
			if(strlen(mechs) > 400)
			{
				for(i = 399;i >= 0;i--)
				{
					if(mechs[i] = ' ')
						mechs[i] = '\0';
					break;
				}
			}

			for(i = 0;mechs[i];i++)
			{
				if(mechs[i] == ' ')
					mechs[i] = ',';
			}

			sasl_sts(p->uid, 'M', mechs);
			free(mechs);
			destroy_session(p);
			return;
		}
		buf[0] = '\0';
	}

	rc = gsasl_step64(p->sctx, buf, &out);
	if(rc == GSASL_OK)
	{
		char *user_name = (char*)gsasl_property_fast(p->sctx, GSASL_AUTHID);
		myuser_t *mu = myuser_find(user_name);
		if(mu && login_user(p, (char*)gsasl_property_fast(p->sctx, GSASL_AUTHID)))
		{
			mu->flags |= MU_SASL;

			if ((md = metadata_find(mu, METADATA_USER, "private:usercloak")))
				cloak = md->value;
			else
				cloak = "*";

			svslogin_sts(p->uid, "*", "*", cloak, mu->name);
			sasl_sts(p->uid, 'D', "S");
		}
		else
			sasl_sts(p->uid, 'D', "F");
		free(out);
		/* Will destroy session on introduction of user to net. */
		return;
	}
	else if(rc == GSASL_NEEDS_MORE)
	{
		if(strlen(out))
			sasl_write(p->uid, out);
		else
			sasl_sts(p->uid, 'C', "+");
		free(out);
		return;
	}

	sasl_sts(p->uid, 'D', "F");
	if(p->sctx)
		gsasl_finish(p->sctx);
	destroy_session(p);
}

static void sasl_write(char *target, char *data)
{
	char out[401];
	int last, rem = strlen(data);

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

int login_user(sasl_session_t *p, char *authid)
{
	myuser_t *mu = myuser_find(authid);
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
	node_t *n;

	slog(LG_INFO, "user_burstlogin");

	/* Not concerned unless its a SASL login. */
	if(p == NULL)
		return;
	destroy_session(p);

	/* WTF? */
	if((mu = u->myuser) == NULL)
		return;

	slog(LG_INFO, "pass!");

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

