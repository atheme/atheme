/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * DH-BLOWFISH mechanism provider
 */

#include "atheme.h"

#ifdef HAVE_OPENSSL

#include <openssl/bn.h>
#include <openssl/dh.h>
#include <openssl/blowfish.h>

DECLARE_MODULE_V1
(
	"saslserv/dh-blowfish", MODULE_UNLOAD_CAPABILITY_NEVER, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static DH *base_dhparams;

mowgli_list_t *mechanisms;
mowgli_node_t *mnode;

static int mech_start(sasl_session_t *p, char **out, int *out_len);
static int mech_step(sasl_session_t *p, char *message, int len, char **out, int *out_len);
static void mech_finish(sasl_session_t *p);
sasl_mechanism_t mech = {"DH-BLOWFISH", &mech_start, &mech_step, &mech_finish};

void _modinit(module_t *m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, mechanisms, "saslserv/main", "sasl_mechanisms");

	if ((base_dhparams = DH_generate_parameters(256, 5, NULL, NULL)) == NULL)
		return;

	mnode = mowgli_node_create();
	mowgli_node_add(&mech, mnode, mechanisms);
}

void _moddeinit(module_unload_intent_t intent)
{
	DH_free(base_dhparams);

	mowgli_node_delete(mnode, mechanisms);
	mowgli_node_free(mnode);
}

static inline DH *DH_clone(DH *dh)
{
	DH *out = DH_new();

	out->p = BN_dup(dh->p);
	out->g = BN_dup(dh->g);

	if (!DH_generate_key(out))
	{
		DH_free(out);
		return NULL;
	}

	return out;
}

static int mech_start(sasl_session_t *p, char **out, int *out_len)
{
	char *ptr;
	DH *dh;

	if ((dh = DH_clone(base_dhparams)) == NULL)
		return ASASL_FAIL;

	/* Serialize p, g, and pub_key */
	*out_len = BN_num_bytes(dh->p) + BN_num_bytes(dh->g) + BN_num_bytes(dh->pub_key) + 6;
	*out = malloc((size_t)*out_len);
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
	if ((mu = myuser_find_by_nick(p->username)) == NULL)
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
	DH_free((DH *) p->mechdata);
	p->mechdata = NULL;
}

#endif /* HAVE_OPENSSL */

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
