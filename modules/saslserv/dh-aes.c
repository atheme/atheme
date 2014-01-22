/*
 * Copyright (c) 2013 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * DH-AES mechanism provider
 * Written by Elizabeth Myers, 14 Apr 2013.
 *
 * Basically the same as DH-BLOWFISH, with a couple alterations:
 * 1) AES instead of blowfish. Blowfish suffers from weak keys, and the author
 *    of it (Bruce Schneier) recommends not using it.
 * 2) Username is encrypted with password
 * 3) CBC mode is used with AES. The IV is sent with the key by the client.
 *
 * Why this over ECDSA-NIST256p-CHALLENGE? Not all langs have good support for
 * ECDSA. DH and AES support, on the other hand, are relatively ubiqutious. It
 * may also be usable in environments where the convenience of a password
 * outweighs the benefits of using challenge-response.
 *
 * Padding structure (sizes in big-endian/network byte order):
 * Server sends: <size p><p><size g><g><size pubkey><pubkey>
 * Client sends: <size pubkey><pubkey><iv (always 16 bytes)><ciphertext>
 *
 * Ciphertext is to be the following encrypted:
 * <username>\0<password>\0<padding if necessary>
 *
 * The DH secret is used to encrypt and decrypt blocks as the key.
 *
 * Note: NEVER reuse IV's. I believe this goes without saying.
 * Also ensure your padding is random. This is just good practise.
 */

#include "atheme.h"

#ifdef HAVE_OPENSSL

#include <openssl/bn.h>
#include <openssl/dh.h>
#include <openssl/aes.h>

DECLARE_MODULE_V1
(
	"saslserv/dh-aes", MODULE_UNLOAD_CAPABILITY_NEVER, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static DH *base_dhparams;

mowgli_list_t *mechanisms;
mowgli_node_t *mnode;

static int mech_start(sasl_session_t *p, char **out, int *out_len);
static int mech_step(sasl_session_t *p, char *message, int len, char **out, int *out_len);
static void mech_finish(sasl_session_t *p);
sasl_mechanism_t mech = {"DH-AES", &mech_start, &mech_step, &mech_finish};

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
	AES_KEY key;
	BIGNUM *their_key = NULL;
	myuser_t *mu;
	char *secret = NULL, *userpw = NULL, *ptr = NULL;
	char iv[AES_BLOCK_SIZE];
	int size, ret = ASASL_FAIL;

	if (!p->mechdata)
		return ASASL_FAIL;
	dh = (DH*)p->mechdata;

	/* Their pub_key */
	if (len <= 2)
		goto end;

	size = ntohs(*(unsigned int*)message);
	message += 2;
	len -= 2;

	if (size >= len)
		goto end;

	if ((their_key = BN_bin2bn(message, size, NULL)) == NULL)
		goto end;

	message += size;
	len -= size;

	/* Data must be a multiple of the AES block size. (16)
	 * Verify we also have an IV and at least one block of data.
	 * Cap at a rather arbitrary limit of 272 (IV + 16 blocks of 16 each).
	 */
	if (len < sizeof(iv) + AES_BLOCK_SIZE || len % AES_BLOCK_SIZE || len > 272)
		goto end;

	/* Extract the IV */
	memcpy(iv, message, sizeof(iv));
	message += sizeof(iv);
	len -= sizeof(iv);

	/* Compute shared secret */
	secret = malloc(DH_size(dh));
	if ((size = DH_compute_key(secret, their_key, dh)) == -1)
		goto end;

	/* Decrypt! (AES_set_decrypt_key takes bits not bytes, hence multiply
	 * by 8) */
	AES_set_decrypt_key(secret, size * 8, &key);

	ptr = userpw = malloc(len + 1);
	userpw[len] = '\0';
	AES_cbc_encrypt(message, userpw, len, &key, iv, AES_DECRYPT);

	/* Username */
	size = strlen(ptr);
	if (size++ >= NICKLEN) /* our base64 routines null-terminate - how polite */
		goto end;
	p->username = strdup(ptr);
	ptr += size;
	len -= size;
	if ((mu = myuser_find_by_nick(p->username)) == NULL)
		goto end;

	/* Password remains */
	if (verify_password(mu, ptr))
		ret = ASASL_DONE;
end:
	if (their_key)
		BN_free(their_key);
	free(secret);
	free(userpw);
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
