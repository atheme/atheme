/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * CRYPT mechanism provider
 *
 */

/******************************* WARNING ******************************************
 * This mechanism presents a vulnerability that allows any user to be logged in   *
 * providing their crytped password is known. This allows attackers with a stolen *
 * DB or crypted password to instantly log in using only the crypted password and *
 * without cracking or brute-forcing. If you use this, guard your DB closely!     *
 **********************************************************************************/

#include "atheme.h"

DECLARE_MODULE_V1
(
	"saslserv/crypt", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

mowgli_list_t *mechanisms;
mowgli_node_t *mnode;
static int mech_start(sasl_session_t *p, char **out, int *out_len);
static int mech_step(sasl_session_t *p, char *message, int len, char **out, int *out_len);
static void mech_finish(sasl_session_t *p);
sasl_mechanism_t mech = {"CRYPT", &mech_start, &mech_step, &mech_finish};

struct crypt_status
{
	unsigned char client_data[16];
	unsigned char server_data[16];
	unsigned char *password;
	unsigned char stage;
};

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(mechanisms, "saslserv/main", "sasl_mechanisms");
	mnode = mowgli_node_create();
	mowgli_node_add(&mech, mnode, mechanisms);
}

void _moddeinit()
{
	mowgli_node_delete(mnode, mechanisms);
}

/* Protocol synopsis;
 * S -> C: 16 random bytes
 * C -> S: 16 random bytes(different from server's random bytes) + username
 * S -> C: salt from user's pass(possibly generated on the spot)
 * C -> S: raw MD5 of (server's data + client's data + crypted pass)
 *
 * WARNING: this allows the client to log in given just the encrypted password
 */

static int mech_start(sasl_session_t *p, char **out, int *out_len)
{
	struct crypt_status *s;
	int i;

	/* Allocate session structure for our crap */
	p->mechdata = malloc(sizeof(struct crypt_status));
	s = (struct crypt_status *)p->mechdata;
	s->stage = 0;
	s->password = NULL;

	/* Generate server's random data */
	for(i = 0;i < 16;i++)
		s->server_data[i] = (unsigned char)(arc4random() % 256);

	/* Send data to client */
	*out = malloc(16);
	memcpy(*out, s->server_data, 16);
	*out_len = 16;

	return ASASL_MORE;
}

static int mech_step(sasl_session_t *p, char *message, int len, char **out, int *out_len)
{
	struct crypt_status *s = (struct crypt_status *)p->mechdata;
	myuser_t *mu;
	s->stage++;

	if(s->stage == 1) /* C -> S: username + 16 bytes random data */
	{
		char user[64];

		if(len < 17)
			return ASASL_FAIL;

		/* Store client's random data & skip to username */
		memcpy(s->client_data, message, 16);
		message += 16;
		len -= 16;

		/* Sanitize and check if user exists */
		strlcpy(user, message, len > 63 ? 64 : len + 1);
		if(!(mu = myuser_find(user)))
			return ASASL_FAIL;
		p->username = strdup(user);

		/* Send salt from password to client, generating one if necessary */
		if(mu->flags & MU_CRYPTPASS)
		{
			if(strlen(mu->pass) == 13) /* original DES type */
			{
				*out_len = 2;
				*out = malloc(2);
				memcpy(*out, mu->pass, 2);
			}
			else if(*(mu->pass) == '$') /* FreeBSD MD5 type */
			{
				*out_len = strlen(mu->pass) - 22;
				*out = malloc(*out_len);
				memcpy(*out, mu->pass, *out_len);
				(*out)[(*out_len) - 1] = '$';
			}
			s->password = (unsigned char *) strdup(mu->pass);
		}
		else
		{
			s->password = (unsigned char *) strdup(crypt(mu->pass, gen_salt()));
			*out_len = 10;
			*out = strdup((char *)s->password);
		}
		return ASASL_MORE;
	}
	else if(s->stage == 2) /* C -> S: raw MD5 of server random data + client random data + crypted password */
	{
		md5_state_t ctx;
		char hash[16];

		if(len != 16)
			return ASASL_FAIL;

		md5_init(&ctx);
		md5_append(&ctx, s->server_data, 16);
		md5_append(&ctx, s->client_data, 16);
		md5_append(&ctx, s->password, strlen((char *)s->password));
		md5_finish(&ctx, (unsigned char *) hash);

		if(!memcmp(message, hash, 16))
			return ASASL_DONE;
		else
			return ASASL_FAIL;
	}else /* wtf? */
		return ASASL_FAIL;
}

static void mech_finish(sasl_session_t *p)
{
	if(p->mechdata)
	{
		struct crypt_status *s = (struct crypt_status *)p->mechdata;
		free(s->password);
		free(p->mechdata);
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
