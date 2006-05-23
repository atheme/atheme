/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * RSA/DSA-SHA1 mechanism provider
 *
 * $Id: pubkey.c 5291 2006-05-23 21:48:03Z jilles $
 */

#include "atheme.h"

#include <openssl/bn.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/objects.h>
#include <arpa/inet.h>

#define NO_DSA /* XXX: implement this :TODO */
/*#define NO_RSA*/

DECLARE_MODULE_V1
(
	"saslserv/pubkey", FALSE, _modinit, _moddeinit,
	"$Id: pubkey.c 5291 2006-05-23 21:48:03Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *mechanisms;

static int mech_start(sasl_session_t *p, char **out, int *out_len);
static int common_step(int using_rsa, sasl_session_t *p, char *message, int len, char **out, int *out_len);
static void mech_finish(sasl_session_t *p);

static unsigned char *hex_to_raw(unsigned char *src, unsigned char *targ);
static unsigned char *raw_to_hex(unsigned char *src); /* Returns pointer to fixed memory. Don't try to free it or anything. */

#ifndef NO_DSA
#include <openssl/dsa.h>
node_t *dsa_node;
static int dsa_step(sasl_session_t *p, char *message, int len, char **out, int *out_len);
static DSA *dsa_from_blob(unsigned char **blob, int *length, char *fpchain);
sasl_mechanism_t dsa_mech = {"DSA-SHA1", &mech_start, &dsa_step, &mech_finish};
#endif

#ifndef NO_RSA
#include <openssl/rsa.h>
node_t *rsa_node;
static int rsa_step(sasl_session_t *p, char *message, int len, char **out, int *out_len);
static RSA *rsa_from_blob(unsigned char **blob, int *length, char *fpchain);
sasl_mechanism_t rsa_mech = {"RSA-SHA1", &mech_start, &rsa_step, &mech_finish};
#endif

void _modinit(module_t *m)
{
	mechanisms = module_locate_symbol("saslserv/main", "sasl_mechanisms");
#ifndef NO_DSA
	dsa_node = node_create();
	node_add(&dsa_mech, dsa_node, mechanisms);
#endif
#ifndef NO_RSA
	rsa_node = node_create();
	node_add(&rsa_mech, rsa_node, mechanisms);
#endif
}

void _moddeinit()
{
#ifndef NO_DSA
	node_del(dsa_node, mechanisms);
	node_free(dsa_node);
#endif
#ifndef NO_RSA
	node_del(rsa_node, mechanisms);
	node_free(rsa_node);
#endif
}

/* Protocol synopsis:
 * S->C: N bytes random data
 * C->S: N bytes random data (same length as server sent!)
 *			+ key blob
 *			+ signature
 *			+ username
 * S: If user is not found, fail.
 * S: If key does not match stored fingerprint, fail.
 * S: Verify signature of (server rand data + client rand data) given the pubkey. If valid, succeed. If invalid, fail.
 *
 * Note on server-side fingerprints:
 * Fingerprints are stored as a chain. The first fingerprint
 * is of the entire key in blob form - this is the minimum
 * fingerprint that client must supply. On first use, the
 * remainder of the fingerprints are calculated from each component
 * of the key individually, in the same order as in the blob -
 * ie for RSA a complete fingerprint chain would consist of
 * sha1(n.len + n + e.len + e) + sha1(n) + sha1(e). This
 * provides for extra security, while taking up less space
 * than the entire pubkey. Users may optionally set the entire
 * fingerprint chain at once for hardcore security.
 */

static int mech_start(sasl_session_t *p, char **out, int *out_len)
{
	int i;

	*out = (unsigned char*)malloc(32);
	*out_len = 32;

	srand(time(NULL));
	for(i = 0;i < 32;i++)
		(*out)[i] = (unsigned char)(rand() % 256);

	p->mechdata = malloc(32);
	memcpy(p->mechdata, *out, 32);

	return ASASL_MORE;
}

#ifndef NO_DSA
static int dsa_step(sasl_session_t *p, char *message, int len, char **out, int *out_len)
{
	return common_step(0, p, message, len, out, out_len);
}
#endif

#ifndef NO_RSA
static int rsa_step(sasl_session_t *p, char *message, int len, char **out, int *out_len)
{
	return common_step(1, p, message, len, out, out_len);
}
#endif

static int common_step(int using_rsa, sasl_session_t *p, char *message, int len, char **out, int *out_len)
{
	myuser_t *mu;
	metadata_t *md;
	unsigned char client_data[32], digest[20];
	unsigned char username[NICKLEN], fpchain[80], *knownchain = NULL;
	char buf[32];
	int sig_size, ret = ASASL_FAIL;
	size_t chain_size, chain_expected_size = using_rsa ? 60 : 80;
	SHA_CTX ctx;

	slog(LG_INFO, "enter common step, rsa: %d, session %p, inlen %d", using_rsa, p, len);

	/* This allocates RSA and DSA structures when allowed, and fails if we're called
	 * with an angorithm we weren't compiled with.
	 */
#ifndef NO_RSA
	RSA *rsa = NULL;
#endif
#ifdef NO_DSA
	if(!using_rsa)
		return ASASL_FAIL;
#else
	DSA *dsa = NULL;
#endif
#ifdef NO_RSA
	if(using_rsa)
		return ASASL_FAIL;
#endif

	slog(LG_INFO, "A %d", len);

	/* Sanity check */
	if(p->mechdata == NULL)
		return ASASL_FAIL;

	slog(LG_INFO, "B %d", len);

	/* Client's random data */
	if(len < 32)
		return ASASL_FAIL;
	slog(LG_INFO, "C %d", len);
	memcpy(client_data, message, 32);
	message += 32;
	len -= 32;

	/* Key blob */
#ifndef NO_DSA
	if(!using_rsa)
	{
		if((dsa = dsa_from_blob(&message, &len, fpchain)) == NULL)
			return ASASL_FAIL;
		sig_size = DSA_size(dsa);
	}
#endif
#ifndef NO_RSA
	if(using_rsa)
	{
		if((rsa = rsa_from_blob((unsigned char**)&message, &len, fpchain)) == NULL)
			return ASASL_FAIL;
		sig_size = RSA_size(rsa);
	}
#endif
	slog(LG_INFO, "D %d", len);

	/* Signature */
	if(len < sig_size)
		goto end;
	slog(LG_INFO, "E %d", len);
	unsigned char *signature;
	signature = (unsigned char*)malloc(sig_size);
	memcpy(signature, message, sig_size);
	message += sig_size;
	len -= sig_size;

	if(len >= NICKLEN)
		goto end;
	slog(LG_INFO, "F %d", len);

	strscpy(username, message, len + 1);

	/* User must exist. */
	if((mu = myuser_find(username)) == NULL)
		goto end;
	slog(LG_INFO, "G");

	/* User must have a fingerprint set. */
	sprintf(buf, "private:sasl:%s-fingerprint", using_rsa ? "rsa" : "dsa");
	if((md = metadata_find(mu, METADATA_USER, buf)) == NULL)
		goto end;
	slog(LG_INFO, "H");

	if(!base64_decode_alloc(md->value, strlen(md->value), (char**)&knownchain, &chain_size) ||
		knownchain == NULL)
		goto end;
	slog(LG_INFO, "I");

	/* Check that the fingerprints match the key */
	if(chain_size != chain_expected_size)
	{
	slog(LG_INFO, "J");
		unsigned char *fullchain = NULL;
		/* Match only the overall fingerprint, the one the user initially supplies. */
		if(memcmp(knownchain, fpchain, 20))
			goto end;
	slog(LG_INFO, "K");

		/* Store the computed portions of the fingerprint chain */
		base64_encode_alloc(fpchain, using_rsa ? 60 : 80, (char**)&fullchain);
		if(fullchain == NULL)
			goto end;
	slog(LG_INFO, "L");
		metadata_add(mu, METADATA_USER, buf, fullchain);
		free(fullchain);
	}else if(memcmp(knownchain, fpchain, chain_expected_size))
		goto end;
	slog(LG_INFO, "M");

	/* Finally, verify the signature */
	SHA1_Init(&ctx);
	SHA1_Update(&ctx, p->mechdata, 32);
	SHA1_Update(&ctx, client_data, 32);
	SHA1_Final(digest, &ctx);
#ifndef NO_DSA
	if(!using_rsa && !DSA_verify(NID_sha1, digest, 20, signature, sig_size, dsa))
		goto end;
#endif
#ifndef NO_RSA
	if(using_rsa && !RSA_verify(NID_sha1, digest, 20, signature, sig_size, rsa))
		goto end;
#endif
	slog(LG_INFO, "N");
	/* Touchdown! */
	ret = ASASL_DONE;
end:
	free(knownchain);
#ifndef NO_DSA
	if(dsa)
		DSA_free(dsa);
#endif
#ifndef NO_RSA
	if(rsa)
		RSA_free(rsa);
#endif
	return ret;
}

static void mech_finish(sasl_session_t *p)
{
	if(p && p->mechdata)
	{
		free(p->mechdata);
		p->mechdata = NULL;
	}
}

/*
static void hex_to_raw(unsigned char *src, unsigned char *targ)
{
	char *p = src;
	int acm = 0, n = 0, i = 0;
	while(*p)
	{
		if(*p >= '0' && *p <= '9')
			acm = (acm << 4) + *p - '0';
		else if(*p >= 'a' && *p <= 'f')
			acm = (acm << 4) + *p - 'a' + 10;
		else if(*p >= 'A' && *p <= 'F')
			acm = (acm << 4) + *p - 'A' + 10;
		else
		{
			if(n)
				targ[i++] = acm;
			acm = 0;
			n = 0;
			p++;
			continue;
		}
		if(++n == 2)
		{
			targ[i++] = acm;
			acm = 0;
			n = 0;
		}
		p++;
	}

	if(n)
		targ[i++] = acm;

	while(i++ < len)
		targ[i] = 0;
}

unsigned char buffer[256];
static unsigned char *raw_to_hex(unsigned char *src, int len)
{
	int i;
	char *p = buffer;
	for(i = 0;i < len;i++)
		p += sprintf(p, "%02x:", src[i]);
	sprintf(p, "%02x", src[len - 1]);
	return buffer;
}
*/

#ifndef NO_DSA
static DSA *dsa_from_blob(unsigned char **blob, int *length, char *fpchain)
{
	uint16_t num_len;
	BIGNUM *p, *q, *g;
	DSA *rsa;
	SHA_CTX ctx;
	const unsigned char *ptr = *blob;
	int x = *length;

	/* N Component */
	if(x < 2)
		return NULL;
	num_len = ntohs(*((uint16_t*)ptr));
	ptr += 2;
	x -=2;

	if(x < num_len || (n = BN_bin2bn(ptr, num_len, NULL)) == NULL)
		return NULL;
	SHA1_Init(&ctx);
	SHA1_Update(&ctx, ptr, num_len);
	SHA1_Final(fpchain + 20, &ctx);
	ptr += num_len;
	x -= num_len;

	/* E Component */
	if(x < 2)
		return NULL;
	num_len = ntohs(*((uint16_t*)ptr));
	ptr += 2;
	x - 2;
	if(x < num_len || (e = BN_bin2bn(ptr, num_len, NULL)) == NULL)
	{
		BN_free(n);
		return NULL;
	}
	SHA1_Init(&ctx);
	SHA1_Update(&ctx, ptr, num_len);
	SHA1_Final(fpchain + 40, &ctx);
	ptr += num_len;
	x -= num_len;

	SHA1_Init(&ctx);
	SHA1_Update(&ctx, *blob, *length - x);
	SHA1_Final(fpchain, &ctx);

	if((rsa = RSA_new()) == NULL)
	{
		BN_free(n);
		BN_free(e);
		return NULL;
	}
	rsa->n = n;
	rsa->e = e;
	return rsa;
}
#endif


#ifndef NO_RSA
static RSA *rsa_from_blob(unsigned char **blob, int *length, char *fpchain)
{
	uint16_t num_len;
	BIGNUM *n, *e;
	RSA *rsa;
	SHA_CTX ctx;
	const unsigned char *orig = *blob;
	int olen = *length;

	/* N Component */
	if(*length < 2)
		return NULL;
	num_len = ntohs(*((uint16_t*)*blob));
	*blob += 2;
	*length -=2;

	if(*length < num_len || (n = BN_bin2bn(*blob, num_len, NULL)) == NULL)
		return NULL;
	SHA1_Init(&ctx);
	SHA1_Update(&ctx, *blob, num_len);
	SHA1_Final(fpchain + 20, &ctx);
	*blob += num_len;
	*length -= num_len;

	/* E Component */
	if(*length < 2)
		return NULL;
	num_len = ntohs(*((uint16_t*)*blob));
	*blob += 2;
	*length - 2;
	if(*length < num_len || (e = BN_bin2bn(*blob, num_len, NULL)) == NULL)
	{
		BN_free(n);
		return NULL;
	}
	SHA1_Init(&ctx);
	SHA1_Update(&ctx, *blob, num_len);
	SHA1_Final(fpchain + 40, &ctx);
	*blob += num_len;
	*length -= num_len;

	SHA1_Init(&ctx);
	SHA1_Update(&ctx, orig, olen);
	SHA1_Final(fpchain, &ctx);

	if((rsa = RSA_new()) == NULL)
	{
		BN_free(n);
		BN_free(e);
		return NULL;
	}
	rsa->n = n;
	rsa->e = e;
	return rsa;
}
#endif

