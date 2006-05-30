/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * RSA/DSA-SHA1 mechanism provider
 *
 * $Id: pubkey.c 5328 2006-05-30 04:01:34Z gxti $
 */

#include "atheme.h"

#ifdef HAVE_OPENSSL

#include <openssl/bn.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/objects.h>
#include <arpa/inet.h>

/*#define NO_DSA*/
/*#define NO_RSA*/

DECLARE_MODULE_V1
(
	"saslserv/pubkey", FALSE, _modinit, _moddeinit,
	"$Id: pubkey.c 5328 2006-05-30 04:01:34Z gxti $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *mechanisms;
list_t *ss_cmdtree, *ss_helptree;

static int mech_start(sasl_session_t *p, char **out, int *out_len);
static int common_step(int using_rsa, sasl_session_t *p, char *message, int len, char **out, int *out_len);
static void mech_finish(sasl_session_t *p);

#ifndef NO_DSA
#include <openssl/dsa.h>
node_t *dsa_node;
static int dsa_step(sasl_session_t *p, char *message, int len, char **out, int *out_len);
static DSA *dsa_from_blob(char **blob, int *length, char *fpchain);
sasl_mechanism_t dsa_mech = {"DSA-SHA1", &mech_start, &dsa_step, &mech_finish};
#endif

#ifndef NO_RSA
#include <openssl/rsa.h>
node_t *rsa_node;
static int rsa_step(sasl_session_t *p, char *message, int len, char **out, int *out_len);
static RSA *rsa_from_blob(char **blob, int *length, char *fpchain);
sasl_mechanism_t rsa_mech = {"RSA-SHA1", &mech_start, &rsa_step, &mech_finish};
#endif

static char *raw_to_hex(unsigned char *src);
static void ss_cmd_pkey(char *origin);

command_t ss_pkey = { "PKEY", "Sets your DSA or RSA fingerprint for public key auth.", AC_NONE, ss_cmd_pkey };

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

	ss_cmdtree = module_locate_symbol("saslserv/main", "ss_cmdtree");
	ss_helptree = module_locate_symbol("saslserv/main", "ss_helptree");
	command_add(&ss_pkey, ss_cmdtree);
	help_addentry(ss_helptree, "PKEY", "help/saslserv/pkey", NULL);
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

	command_delete(&ss_pkey, ss_cmdtree);
	help_delentry(ss_helptree, "PKEY");
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
 * Key blobs are composed of one or more binary encoded bignum ints,
 * prefixed by an unsigned big endian 16 bit short integer specifying
 * the length in bytes of the number as encoded. Order is (n, e) for RSA
 * and (p, q, g) for DSA.
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
	char client_data[32], digest[20];
	char username[NICKLEN], fpchain[100];
	char *signature = NULL, *knownchain = NULL;
	char buf[32];
	int sig_size, ret = ASASL_FAIL;
	size_t chain_size, chain_expected_size = using_rsa ? 60 : 100;
	SHA_CTX ctx;

	/* This allocates RSA and DSA structures when allowed, and fails if we're called
	 * with an algorithm we weren't compiled with.
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

	/* Sanity check */
	if(p->mechdata == NULL)
		return ASASL_FAIL;

	/* Client's random data */
	if(len < 32)
		return ASASL_FAIL;
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
		if((rsa = rsa_from_blob(&message, &len, fpchain)) == NULL)
			return ASASL_FAIL;
		sig_size = RSA_size(rsa);
	}
#endif

	/* Signature */
	if(!using_rsa) /* DSA form includes a length for the signature since it != key size */
	{
		if(len < 2)
			goto end;
		sig_size = ntohs(*(size_t*)message);
		message += 2;
		len -= 2;
	}
	if(len < sig_size)
		goto end;
	signature = (char*)malloc(sig_size);
	memcpy(signature, message, sig_size);
	message += sig_size;
	len -= sig_size;

	if(len >= NICKLEN)
		goto end;

	strscpy(username, message, len + 1);

	/* User must exist. */
	if((mu = myuser_find(username)) == NULL)
		goto end;

	/* User must have a fingerprint set. */
	if((md = metadata_find(mu, METADATA_USER, "private:sasl:pkey-fingerprint")) == NULL)
		goto end;

	if(!base64_decode_alloc(md->value, strlen(md->value), &knownchain, &chain_size) ||
		knownchain == NULL)
		goto end;

	/* Check that the fingerprints match the key */
	if(chain_size != chain_expected_size)
	{
		char *fullchain = NULL;
		/* Match only the overall fingerprint, the one the user initially supplies. */
		if(memcmp(knownchain, fpchain, 20))
			goto end;

		/* Store the computed portions of the fingerprint chain */
		base64_encode_alloc(fpchain, using_rsa ? 60 : 80, &fullchain);
		if(fullchain == NULL)
			goto end;
		metadata_add(mu, METADATA_USER, "private:sasl:pkey-fingerprint", fullchain);
		free(fullchain);
	}else if(memcmp(knownchain, fpchain, chain_expected_size))
		goto end;

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

	/* Touchdown! */
	p->username = strdup(username);
	ret = ASASL_DONE;
end:
	free(knownchain);
	free(signature);
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

static void hex_to_raw(char *src, unsigned char *targ)
{
	char *p = src;
	int acm = 0, n = 0, i = 0;
	while(*p)
	{
		if(i >= 20)
			break;
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

	while(i++ < 20)
		targ[i] = 0;
}

unsigned char buffer[256];
static char *raw_to_hex(unsigned char *src)
{
	int i;
	char *p = buffer;
	for(i = 0;i < 19;i++)
		p += sprintf(p, "%02x:", src[i]);
	sprintf(p, "%02x", src[19]);
 	return buffer;
}

static BIGNUM *bignum_from_blob(char **blob, int *length, char *fpchain, int *n)
{
	uint16_t num_len;
	BIGNUM *bn;
	SHA_CTX ctx;

	if(*length < 2)
		return NULL;
	num_len = ntohs(*((uint16_t*)*blob));
	*blob += 2;
	*length -= 2;

	if(*length < num_len || (bn = BN_bin2bn(*blob, num_len, NULL)) == NULL)
		return NULL;
	SHA1_Init(&ctx);
	SHA1_Update(&ctx, *blob, num_len);
	SHA1_Final(fpchain + 20 * ++(*n), &ctx);
	*blob += num_len;
	*length -= num_len;

	return bn;
}

#ifndef NO_DSA
static DSA *dsa_from_blob(char **blob, int *length, char *fpchain)
{
	BIGNUM *p, *q, *g, *y;
	DSA *dsa;
	SHA_CTX ctx;
	const char *orig = *blob;
	int olen = *length;
	int i = 0;

	if((p = bignum_from_blob(blob, length, fpchain, &i)) == NULL) /* P Component */
		return NULL;

	if((q = bignum_from_blob(blob, length, fpchain, &i)) == NULL) /* Q Component */
	{
		BN_free(p);
		return NULL;
	}

	if((g = bignum_from_blob(blob, length, fpchain, &i)) == NULL) /* G Component */
	{
		BN_free(p);
		BN_free(q);
		return NULL;
	}

	if((y = bignum_from_blob(blob, length, fpchain, &i)) == NULL) /* Public Key */
	{
		BN_free(p);
		BN_free(q);
		BN_free(g);
		return NULL;
	}

	SHA1_Init(&ctx);
	SHA1_Update(&ctx, orig, olen - *length);
	SHA1_Final(fpchain, &ctx);

	if((dsa = DSA_new()) == NULL)
	{
		BN_free(p);
		BN_free(q);
		BN_free(g);
		BN_free(y);
		return NULL;
	}
	dsa->p = p;
	dsa->q = q;
	dsa->g = g;
	dsa->pub_key = y;
	return dsa;

}
#endif


#ifndef NO_RSA
static RSA *rsa_from_blob(char **blob, int *length, char *fpchain)
{
	BIGNUM *n, *e;
	RSA *rsa;
	SHA_CTX ctx;
	const char *orig = *blob;
	int olen = *length;
	int i = 0;

	if((n = bignum_from_blob(blob, length, fpchain, &i)) == NULL) /* N Component */
		return NULL;

	if((e = bignum_from_blob(blob, length, fpchain, &i)) == NULL) /* E Component */
	{
		BN_free(n);
		return NULL;
	}

	SHA1_Init(&ctx);
	SHA1_Update(&ctx, orig, olen - *length);
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

static void ss_cmd_pkey(char *origin)
{
	user_t *u = user_find_named(origin);
	myuser_t *mu;
	metadata_t *md;
	char *encoded, *fingerprint = strtok(NULL, "");
	char buf[20];
	size_t len;

	if((mu = u->myuser) == NULL)
	{
		notice(saslsvs.nick, origin, "You are not logged in.");
		return;
	}

	if(fingerprint != NULL)
	{
		memset(buf, 0, 20);
		hex_to_raw(fingerprint, buf);
		base64_encode_alloc(buf, 20, &encoded);
		metadata_add(mu, METADATA_USER, "private:sasl:pkey-fingerprint", encoded);
		notice(saslsvs.nick, origin, "Your DSA/RSA authentication fingerprint has been set to \002%s\002", raw_to_hex(buf));
	}else{
		if((md = metadata_find(mu, METADATA_USER, "private:sasl:pkey-fingerprint")) != NULL)
		{
			if(base64_decode_alloc(md->value, strlen(md->value), &encoded, &len) && len >= 20)
			{
				notice(saslsvs.nick, origin, "Your DSA/RSA authentication fingerprint is \002%s\002", raw_to_hex(encoded));
				return;
			}
			else
				metadata_delete(mu, METADATA_USER, "private:sasl:pkey-fingerprint");
		}

		notice(saslsvs.nick, origin, "You do not have a fingerprint set for this account.");
		notice(saslsvs.nick, origin, "Syntax: PKEY <fingerprint>");
	}
}

#endif /* HAVE_OPENSSL */
