/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Provides a SASL authentication agent for clients.
 *
 * $Id: sasl.c 5123 2006-04-23 03:10:00Z gxti $
 */

/* sasl.h and friends are included from atheme.h now --nenolod */
#include "atheme.h"

DECLARE_MODULE_V1
(
	"saslserv/sasl", FALSE, _modinit, _moddeinit,
	"$Id: sasl.c 5123 2006-04-23 03:10:00Z gxti $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t sessions[HASHSIZE];
list_t *mechanisms;
static BlockHeap *session_heap;

sasl_session_t *find_session(char *uid);
sasl_session_t *make_session(char *uid);
void destroy_session(sasl_session_t *p);
static void sasl_input(void *vptr);
static void sasl_packet(sasl_session_t *p, char *buf, int len);
static void sasl_write(char *target, char *data, int length);
int login_user(sasl_session_t *p);
static void user_burstlogin(void *vptr);

void _modinit(module_t *m)
{
	session_heap = BlockHeapCreate(sizeof(sasl_session_t), 64);
	mechanisms = module_locate_symbol("saslserv/main", "sasl_mechanisms");

	hook_add_event("sasl_input");
	hook_add_hook("sasl_input", sasl_input);
	hook_add_event("user_burstlogin");
	hook_add_hook("user_burstlogin", user_burstlogin);
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
	if(p->mechptr)
		p->mechptr->mech_finish(p); /* Free up any mechanism data */
	free(p->username);
	p->mechptr = NULL; /* We're not freeing the mechanism, just "dereferencing" it */

	BlockHeapFree(session_heap, p);
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

static sasl_mechanism_t *find_mechanism(char *name)
{
	node_t *n;
	sasl_mechanism_t *mptr;

	LIST_FOREACH(n, mechanisms->head)
	{
		mptr = n->data;
		if(!strcmp(mptr->name, name))
			return mptr;
	}

	return NULL;
}

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

			LIST_FOREACH(n, mechanisms->head)
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
	node_t *n;

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

