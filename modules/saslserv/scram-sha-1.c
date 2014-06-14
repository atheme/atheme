/*
 * Copyright (c) 2014 Mantas Mikulėnas <grawity@gmail.com>.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "atheme.h"

#ifdef HAVE_OPENSSL

DECLARE_MODULE_V1
(
	"saslserv/scram-sha-1", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

#include <openssl/sha.h>
#include <openssl/hmac.h>

sasl_mech_register_func_t *regfuncs;
static int mech_start(sasl_session_t *p, char **out, size_t *out_len);
static int mech_step(sasl_session_t *p, char *message, size_t len, char **out, size_t *out_len);
static void mech_finish(sasl_session_t *p);
sasl_mechanism_t mech = {"SCRAM-SHA-1", &mech_start, &mech_step, &mech_finish};

typedef enum {
	SCRAM_CLIENT_FIRST = 0,	/* received 'client-first', about to send 'server-first' */
	SCRAM_CLIENT_FINAL,	/* received 'client-final', about to send 'server-final' */
	SCRAM_PASSED,		/* send 'v=...', received whatever, about to send ASASL_DONE */
	SCRAM_FAILED,		/* sent 'e=...', received whatever, about to send ASASL_FAIL */
} scram_step_t;

typedef struct {
	scram_step_t state;
	myuser_t *mu;

	char *salted_pass;
	char *client_nonce;
	char *server_nonce;

	size_t client_gs2_len;
	char *client_gs2_header;
	size_t client_first_len;
	char *client_first_message;
	size_t server_first_len;
	char *server_first_message;
} scram_session_t;

typedef char* scram_attrlist_t[128];

void _modinit(module_t *m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, regfuncs, "saslserv/main", "sasl_mech_register_funcs");
	regfuncs->mech_register(&mech);
}

void _moddeinit(module_unload_intent_t intent)
{
	regfuncs->mech_unregister(&mech);
}

static int attrlist_parse(char *str, size_t len, scram_attrlist_t *attr)
{
	char *end = str + len;

	for (;;) {
		char name, *value;

		if (str < end && *str == ',')
			str++;

		if (str < end) {
			name = *str++;
			if (name < 'A' || (name > 'Z' && name < 'a') || name > 'z')
				return false;
		} else
			return false;

		if (str < end && *str++ == '=') {
			char *pos = memchr(str, ',', end - str);
			if (pos) {
				(*attr)[name] = sstrndup(str, pos - str); /* freed by caller */
				str = pos;
			} else {
				(*attr)[name] = sstrndup(str, end - str); /* freed by caller */
				return true;
			}
		} else
			return false;
	}

	return true;
}

static void attrlist_free(scram_attrlist_t *attr)
{
	int i;

	if (!attr || !*attr)
		return;

	for (i = 'A'; i <= 'z'; i++)
		free((*attr)[i]);
}

static char * parse_pwhash(const char *str, char **salt, int *rounds)
{
	char *pos, *buf, *err;
	size_t len, rs;

	if (strncmp(str, "$s1$", 4) != 0)
		return NULL;
	str += 4;

	/* get the salt */
	pos = strchr(str, '$');
	if (!pos)
		return NULL;
	len = pos - str;
	*salt = smalloc(len*2); /* freed by caller */
	rs = base64_encode(str, len, *salt, len*2);
	if (rs < 0)
		return NULL;
	str += len + 1;

	/* get the rounds */
	pos = strchr(str, '$');
	if (!pos)
		return NULL;
	len = pos - str;
	buf = sstrndup(str, len);
	*rounds = strtoul(buf, &err, 10);
	free(buf);
	if (err && *err)
		return NULL;
	str += len + 1;

	/* return the hash */
	buf = smalloc(SHA_DIGEST_LENGTH + 4); /* freed by mech_finish */
	rs = base64_decode(str, buf, SHA_DIGEST_LENGTH + 4);
	if (rs != SHA_DIGEST_LENGTH) {
		free(buf);
		return NULL;
	}
	return buf;
}

static int mech_start(sasl_session_t *p, char **out, size_t *out_len)
{
	scram_session_t *s = mowgli_alloc(sizeof(scram_session_t));
	p->mechdata = s;

	return ASASL_MORE;
}

static int _step_client_first(sasl_session_t *p, char *message, size_t len, char **out, size_t *out_len)
{
	scram_session_t *s = p->mechdata;
	scram_attrlist_t input = { 0 };

	char *header = NULL;
	char *end = message + len;

	char *rand = NULL;
	char *salt = NULL;
	int rounds = -1;

	/* parse gs2-header */

	header = message;

	switch (message[0]) {
	case 'y':
	case 'n':
		message++;
		break;
	case 'p':
		/* we cannot support TLS channel bindings over IRC network */
		return ASASL_FAIL;
	default:
		return ASASL_FAIL;
	}

	if (message[0] == ',')
		message++;
	else
		return ASASL_FAIL;

	if (message[0] == 'a' && message[1] == '=') {
		char *pos;
		message += 2;
		pos = memchr(message, ',', len);
		if (!pos)
			return ASASL_FAIL;
		p->authzid = sstrndup(message, pos - message); /* freed by SaslServ */
		message = pos + 1;
	}
	else if (message[0] == ',')
		message += 1;
	else
		return ASASL_FAIL;

	/* parse the 'client-first-message-bare' */

	s->client_gs2_len = message - header;
	s->client_gs2_header = sstrndup(header, message - header); /* freed by mech_finish */
	s->client_first_len = len;
	s->client_first_message = sstrndup(message, len); /* freed by mech_finish */

	if (!attrlist_parse(message, len, &input)) {
		slog(LG_DEBUG, "scram-sha-1: failed to parse 'client-first'");
		return ASASL_FAIL;
	}

	if (input['m'] ||
	    !input['n'] || !*input['n'] ||
	    !input['r'] || !*input['r']) {
		slog(LG_DEBUG, "scram-sha-1: missing parameters in 'client-first'");
		return ASASL_FAIL;
	}

	p->username     = sstrdup(input['n']); /* freed by SaslServ */
	s->client_nonce = sstrdup(input['r']); /* freed by mech_finish */

	/* get user's PBKDF2 salt */

	s->mu = myuser_find(p->username);
	if (!s->mu)
		goto err_free;

	if (s->mu->flags & MU_CRYPTPASS)
		s->salted_pass = parse_pwhash(s->mu->pass, &salt, &rounds);
	else
		s->salted_pass = parse_pwhash(crypt_string(s->mu->pass, gen_salt()), &salt, &rounds);

	if (!s->salted_pass) {
		slog(LG_ERROR, "scram-sha-1: malformed password hash (not pbkdf2_sha1) for %s[%s]",
			entity(s->mu)->name, entity(s->mu)->id);
		goto err_free;
	}

	/* TODO: stored_key and server_key should be obtained here,
	 * rather than just salted_pass
	 */

	/* generate server nonce */

	s->server_nonce = smalloc(strlen(s->client_nonce) + 16 + 1); /* freed by mech_finish */

	rand = random_string(16);
	strcpy(s->server_nonce, s->client_nonce);
	strcat(s->server_nonce, rand);
	free(rand);

	/* send the 'server-first' message */

	*out = smalloc(400); /* freed by SaslServ */
	*out_len = snprintf(*out, 400, "r=%s,s=%s,i=%u",
			s->server_nonce,
			salt,
			rounds);

	s->server_first_message = sstrdup(*out); /* freed by mech_finish */
	s->server_first_len = *out_len;
	s->state = SCRAM_CLIENT_FINAL;

	free(salt);

	return ASASL_MORE;

err_free:
	free(salt);
	attrlist_free(&input);

	return ASASL_FAIL;
}

static int _step_client_final(sasl_session_t *p, char *message, size_t len, char **out, size_t *out_len)
{
	scram_session_t *s = p->mechdata;
	scram_attrlist_t input = { 0 };

	char *recv_header = NULL;

	size_t auth_len;
	char *auth_message = NULL;

	char client_key[SHA_DIGEST_LENGTH];
	char stored_key[SHA_DIGEST_LENGTH];
	char client_sig[SHA_DIGEST_LENGTH];
	char client_proof[SHA_DIGEST_LENGTH];
	char recv_proof[SHA_DIGEST_LENGTH+4];
	char server_key[SHA_DIGEST_LENGTH];
	char server_sig[SHA_DIGEST_LENGTH];

	int i;
	char *rc;
	size_t rs;

	/* parse input */

	if (strnlen(message, len) != len) {
		slog(LG_DEBUG, "scram-sha-1: embedded nulls in client-final");
		goto err_free;
	}

	if (!attrlist_parse(message, len, &input)) {
		slog(LG_DEBUG, "scram-sha-1: failed to parse client-final");
		goto err_free;
	}

	if (input['m']) {
		slog(LG_DEBUG, "scram-sha-1: unsupported extensions in client-final");
		goto err_free;
	}

	if (!input['c'] || !*input['c'] ||
	    !input['r'] || !*input['r'] ||
	    !input['p'] || !*input['p']) {
		slog(LG_DEBUG, "scram-sha-1: missing parameters in client-final");
		goto err_free;
	}

	/* verify received nonce */

	if (strcmp(input['r'], s->server_nonce) != 0) {
		slog(LG_DEBUG, "scram-sha-1: client nonce mismatch");
		goto err_free;
	}

	/* verify channel binding header; as we refuse channel bindings in
	 * first step, we only need to compare the GS2 header
	 */

	recv_header = smalloc(128); /* freed here */
	rs = base64_decode(input['c'], recv_header, 128);
	if (rs < 0) {
		slog(LG_DEBUG, "scram-sha-1: failed to decode received gs2_header");
		goto err_free;
	}
	if (rs != s->client_gs2_len ||
	    strncmp(recv_header, s->client_gs2_header, rs) != 0) {
		slog(LG_DEBUG, "scram-sha-1: client channel binding header mismatch");
		goto err_free;
	}

	/* decode received proof for verifying */

	rs = base64_decode(input['p'], recv_proof, SHA_DIGEST_LENGTH+4);
	if (rs != SHA_DIGEST_LENGTH) {
		slog(LG_DEBUG, "scram-sha-1: failed to decode received proof");
		goto err_free;
	}

	/* generate AuthMessage */

	auth_len = s->client_first_len + s->server_first_len + len + 3;
	auth_message = smalloc(auth_len); /* freed here */
	auth_len = snprintf(auth_message, auth_len, "%s,%s,c=%s,r=%s",
				s->client_first_message,
				s->server_first_message,
				input['c'],
				input['r']);
	if (auth_len <= 0) {
		slog(LG_ERROR, "scram-sha-1: failed to format auth_message");
		goto err_free;
	}

	/* TODO: StoredKey and ServerKey (along with salt & rounds) should be
	 * stored as metadata, rather than recalculating from salted_pass, to
	 * permit other password storage mechanisms. Need to add hooks in
	 * set_password() and verify_password() maybe?
	 *
	 * The proof check would be changed
	 * from "proof = c_key ^ c_sig ; require proof == received_proof"
	 * to "c_key = c_sig ^ proof; require H(c_key) == stored_key".
	 */

#if 0

	/* obtain StoredKey from metadata */

	/* calculate ClientSignature */

	rc = HMAC(EVP_sha1(),
		our_stored_key, SHA_DIGEST_LENGTH,
		auth_message, auth_len,
		our_client_sig, NULL);
	if (!rc) {
		slog(LG_ERROR, "scram-sha-1: HMAC() failed when calculating client_sig");
		goto err_free;
	}

	/* recover received ClientKey */

	for (i = 0; i < SHA_DIGEST_LENGTH; i++)
		recv_client_key[i] = our_client_sig[i] ^ recv_proof[i];

	/* calculate received StoredKey */

	rc = SHA1(recv_client_key, SHA_DIGEST_LENGTH, recv_stored_key);
	if (!rc) {
		slog(LG_ERROR, "scram-sha-1: SHA1() failed when calculating stored_key");
		goto err_free;
	}

	if (memcmp(recv_stored_key, our_stored_key, SHA_DIGEST_LENGTH) != 0) {
		slog(LG_DEBUG, "scram-sha-1: stored-key mismatch");
		goto err_free;
	}

	/* obtain ServerKey from metadata */

#else

	/* calculate ClientKey */

	rc = HMAC(EVP_sha1(),
		s->salted_pass, SHA_DIGEST_LENGTH,
		"Client Key", strlen("Client Key"),
		client_key, NULL);
	if (!rc) {
		slog(LG_ERROR, "scram-sha-1: HMAC() failed when calculating client_key");
		goto err_free;
	}

	/* calculate StoredKey */

	rc = SHA1(client_key, SHA_DIGEST_LENGTH, stored_key);
	if (!rc) {
		slog(LG_ERROR, "scram-sha-1: SHA1() failed when calculating stored_key");
		goto err_free;
	}

	/* calculate ClientSignature */

	rc = HMAC(EVP_sha1(),
		stored_key, SHA_DIGEST_LENGTH,
		auth_message, auth_len,
		client_sig, NULL);
	if (!rc) {
		slog(LG_ERROR, "scram-sha-1: HMAC() failed when calculating client_sig");
		goto err_free;
	}

	/* calculate and verify ClientProof */

	for (i = 0; i < SHA_DIGEST_LENGTH; i++)
		client_proof[i] = client_key[i] ^ client_sig[i];

	if (memcmp(recv_proof, client_proof, SHA_DIGEST_LENGTH) != 0) {
		slog(LG_DEBUG, "scram-sha-1: client proof mismatch");
		goto err_free;
	}

	/* calculate ServerKey */

	rc = HMAC(EVP_sha1(),
		s->salted_pass, SHA_DIGEST_LENGTH,
		"Server Key", strlen("Server Key"),
		server_key, NULL);
	if (!rc) {
		slog(LG_ERROR, "scram-sha-1: HMAC() failed when calculating server_key");
		goto err_free;
	}

#endif

	/* calculate ServerSignature */

	rc = HMAC(EVP_sha1(),
		server_key, SHA_DIGEST_LENGTH,
		auth_message, auth_len,
		server_sig, NULL);
	if (!rc) {
		slog(LG_ERROR, "scram-sha-1: HMAC() failed when calculating server_sig");
		goto err_free;
	}

	/* send ServerSignature */

	*out = smalloc(42); /* freed by SaslServ */
	(*out)[0] = 'v';
	(*out)[1] = '=';
	*out_len = base64_encode(server_sig, SHA_DIGEST_LENGTH, (*out)+2, 40);
	if (*out_len < 0) {
		slog(LG_ERROR, "scram-sha-1: ran out of space when encoding server_sig");
		goto err_free;
	}
	*out_len += 2;

	s->state = SCRAM_PASSED;

	free(auth_message);
	return ASASL_MORE;

err_free:
	free(auth_message);
	free(recv_header);
	attrlist_free(&input);

	return ASASL_FAIL;
}

static int mech_step(sasl_session_t *p, char *message, size_t len, char **out, size_t *out_len)
{
	scram_session_t *s = p->mechdata;

	switch (s->state) {
	case SCRAM_CLIENT_FIRST:
		return _step_client_first(p, message, len, out, out_len);
	case SCRAM_CLIENT_FINAL:
		return _step_client_final(p, message, len, out, out_len);
	case SCRAM_PASSED:
		return ASASL_DONE;
	case SCRAM_FAILED:
		return ASASL_FAIL;
	}
}

static void mech_finish(sasl_session_t *p)
{
	if (p->mechdata) {
		scram_session_t *s = p->mechdata;
		free(s->salted_pass);
		free(s->client_nonce);
		free(s->server_nonce);
		free(s->client_gs2_header);
		free(s->client_first_message);
		free(s->server_first_message);
		mowgli_free(s);
		p->mechdata = NULL;
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */

#endif
