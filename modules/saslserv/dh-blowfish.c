/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * DH-BLOWFISH mechanism provider
 *
 */

#include "atheme.h"

#ifdef HAVE_OPENSSL

#include <openssl/bn.h>
#include <openssl/dh.h>
#include <openssl/blowfish.h>
#include <arpa/inet.h>

DECLARE_MODULE_V1
(
	"saslserv/dh-blowfish", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

mowgli_list_t *mechanisms;
mowgli_node_t *mnode;

static int mech_start(sasl_session_t *p, char **out, int *out_len);
static int mech_step(sasl_session_t *p, char *message, int len, char **out, int *out_len);
static void mech_finish(sasl_session_t *p);
sasl_mechanism_t mech = {"DH-BLOWFISH", &mech_start, &mech_step, &mech_finish};

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(mechanisms, "saslserv/main", "sasl_mechanisms");
	mnode = mowgli_node_create();
	mowgli_node_add(&mech, mnode, mechanisms);
}

void _moddeinit()
{
	mowgli_node_delete(mnode, mechanisms);
	mowgli_node_free(mnode);
}

static int mech_start(sasl_session_t *p, char **out, int *out_len)
{
	DH *dh;
	char *ptr;

	if ((dh = DH_generate_parameters(256, 5, NULL, NULL)) == NULL)
		return ASASL_FAIL;

	if (!DH_generate_key(dh))
	{
		DH_free(dh);
		return ASASL_FAIL;
	}

	/* Serialize p, g, and pub_key */
	*out = malloc(BN_num_bytes(dh->p) + BN_num_bytes(dh->g) + BN_num_bytes(dh->pub_key) + 6);
	*out_len = BN_num_bytes(dh->p) + BN_num_bytes(dh->g) + BN_num_bytes(dh->pub_key) + 6;
	ptr = *out;

	/* p */
	*((unsigned int *)ptr) = htons(BN_num_bytes(dh->p));
	BN_bn2bin(dh->p, (unsigned char *)ptr + 2);
	ptr += 2 + BN_num_bytes(dh->p);

	/* g */
	*((unsigned int *)ptr) = htons(BN_num_bytes(dh->g));
	BN_bn2bin(dh->g, (unsigned char *)ptr + 2);
	ptr += 2 + BN_num_bytes(dh->g);

	/* pub_key */
	*((unsigned int *)ptr) = htons(BN_num_bytes(dh->pub_key));
	BN_bn2bin(dh->pub_key, (unsigned char *)ptr + 2);
	ptr += 2 + BN_num_bytes(dh->pub_key);

	p->mechdata = dh;
	return ASASL_MORE;
}

static int mech_step(sasl_session_t *p, char *message, int len, char **out, int *out_len)
{
	DH *dh = NULL;
	BF_KEY key;
	BIGNUM *their_key = NULL;
	myuser_t *mu;
	char *ptr, *secret = NULL, *password = NULL;
	int size, ret = ASASL_FAIL;

	if (!p->mechdata)
		return ASASL_FAIL;
	dh = (DH*)p->mechdata;

	/* Their pub_key */
	if (len < 2)
		goto end;
	size = ntohs(*(unsigned int*)message);
	message += 2;
	len -= 2;
	if (size > len)
		goto end;
	if ((their_key = BN_bin2bn((unsigned char *)message, size, NULL)) == NULL)
		goto end;
	message += size;
	len -= size;
	
	/* Username */
	size = strlen(message);
	if (size >= NICKLEN) /* our base64 routines null-terminate - how polite */
		goto end;
	p->username = strdup(message);
	message += size + 1;
	len -= size + 1;
	if ((mu = myuser_find(p->username)) == NULL)
		goto end;
	/* AES-encrypted password remains */

	/* Compute shared secret */
	secret = (char*)malloc(DH_size(dh));
	if ((size = DH_compute_key((unsigned char *)secret, their_key, dh)) == -1)
		goto end;

	/* Data must be multiple of block size, and let's be reasonable about size */
	if (len == 0 || len % 8 || len > 128)
		goto end;

	/* Decrypt! */
	BF_set_key(&key, size, (unsigned char *)secret);
	ptr = password = (char*)malloc(len + 1);
	password[len] = '\0';
	while (len)
	{
		BF_ecb_encrypt((unsigned char *)message, (unsigned char *)ptr, &key, BF_DECRYPT);
		message += 8;
		ptr += 8;
		len -= 8;
	}

	if (verify_password(mu, password))
		ret = ASASL_DONE;
	
end:
	if (their_key)
		BN_free(their_key);
	free(secret);
	free(password);
	return ret;
}

static void mech_finish(sasl_session_t *p)
{
	if (p && p->mechdata)
	{
		DH_free((DH*)p->mechdata);
		p->mechdata = NULL;
	}
}

#endif /* HAVE_OPENSSL */

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
