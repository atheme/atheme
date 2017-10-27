/*
 * SCRAM-SHA-1 and SCRAM-SHA-256 SASL mechanisms provider.
 *
 * See the following RFCs for details:
 *
 *   - RFC 5802 <https://tools.ietf.org/html/rfc5802>
 *     "Salted Challenge Response Authentication Mechanism (SCRAM)"
 *
 *   - RFC 7677 <https://tools.ietf.org/html/rfc7677>
 *     "SCRAM-SHA-256 and SCRAM-SHA-256-PLUS SASL Mechanisms"
 *
 * Copyright (c) 2014 Mantas Mikulėnas <grawity@gmail.com>
 * Copyright (c) 2017 Aaron M. D. Jones <aaronmdjones@gmail.com>
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

#include <openssl/evp.h>
#include <openssl/hmac.h>

#include "pbkdf2v2.h"

#define NONCE_LENGTH                32          // This should be more than sufficient
#define RESPONSE_LENGTH             372         // (372 * 1.333) + strlen("AUTHENTICATE \r\n") == 511

enum scramsha_step
{
	SCRAMSHA_STEP_CLIENTFIRST   = 0,        // Waiting for client-first-message
	SCRAMSHA_STEP_CLIENTPROOF   = 1,        // Waiting for client-final-message
	SCRAMSHA_STEP_PASSED        = 2,        // Authentication has succeeded
	SCRAMSHA_STEP_FAILED        = 3,        // Authentication has failed
};

struct scramsha_session
{
	struct pbkdf2v2_parameters  db;         // Parsed credentials from database
	myuser_t                   *mu;         // User we are authenticating as
	char                       *cn;         // Client nonce
	char                       *sn;         // Server nonce
	char                       *c_gs2_buf;  // Client's GS2 header
	char                       *c_msg_buf;  // Client's first message
	char                       *s_msg_buf;  // Server's first message
	size_t                      c_gs2_len;  // Client's GS2 header (length)
	size_t                      c_msg_len;  // Client's first message (length)
	size_t                      s_msg_len;  // Server's first message (length)
	enum scramsha_step          step;       // What step in authentication are we at?
};

typedef char *scram_attr_list[128];

static atheme_pbkdf2v2_scram_ex_fn sasl_scramsha_ex = NULL;

static int
sasl_scramsha_attrlist_parse(const char *restrict str, scram_attr_list *const restrict attrs)
{
	const char *const end = str + strlen(str);

	for (;;)
	{
		if (str < end && *str == ',')
			str++;

		if (str < end)
		{
			unsigned char name = (unsigned char) *str++;

			if (name < 'A' || (name > 'Z' && name < 'a') || name > 'z')
				// RFC 5802 Section 5: "All attribute names are single US-ASCII letters"
				return false;

			if (str < end && *str++ == '=')
			{
				const char *const pos = memchr(str, ',', (size_t) (end - str));

				if (pos)
				{
					(*attrs)[name] = strndup(str, (size_t) (pos - str));
					str = pos;
				}
				else
				{
					(*attrs)[name] = strndup(str, (size_t) (end - str));

					// Reached end of attribute list
					return true;
				}
			}
			else
				// Attributes must have values
				return false;
		}
		else
			// Reached end of attribute list without parsing valid attribute last
			return false;
	}
}

static inline void
sasl_scramsha_attrlist_free(scram_attr_list *const restrict attrs)
{
	for (unsigned char x = 'A'; x <= 'z'; x++)
	{
		(void) free((*attrs)[x]);

		(*attrs)[x] = NULL;
	}
}

static int
sasl_scramsha_start(sasl_session_t *const restrict p, char __attribute__((unused)) **const restrict out,
                    size_t __attribute__((unused)) *const restrict out_len)
{
	p->mechdata = calloc(1, sizeof(struct scramsha_session));

	return ASASL_MORE;
}

static void
sasl_scramsha_finish(sasl_session_t *const restrict p)
{
	if (p && p->mechdata)
	{
		struct scramsha_session *const s = p->mechdata;

		(void) free(s->cn);
		(void) free(s->sn);
		(void) free(s->c_gs2_buf);
		(void) free(s->c_msg_buf);
		(void) free(s->s_msg_buf);
		(void) free(s);

		p->mechdata = NULL;
	}
}

static int
sasl_scramsha_step_clientfirst(sasl_session_t *const restrict p, char *const restrict data, const size_t len,
                               char **restrict out, size_t *const restrict out_len, const unsigned int prf)
{
	struct scramsha_session *const s = p->mechdata;

	const char *const header = data;
	const char *message = data;

	scram_attr_list input;
	(void) memset(input, 0x00, sizeof input);

	if (strnlen(data, len) != len)
		// NULL byte in data received from client
		goto fail;

	switch (message[0])
	{
		// RFC 5802 Section 7 (gs2-cbind-flag)
		case 'y':
		case 'n':
			message++;
			break;

		case 'p':
			// We don't support channel binding
			goto fail;

		default:
			// Invalid first byte
			goto fail;
	}

	if (message[0] == ',')
		message++;
	else
		// Malformed GS2 header
		goto fail;

	// Does GS2 header include an authzid ?
	if (message[0] == 'a' && message[1] == '=')
	{
		/*
		 * TODO: Normalise the username
		 */

		message += 2;

		// Locate end of authzid
		const char *const pos = strchr(message + 1, ',');
		if (! pos)
			// Malformed GS2 header
			goto fail;

		// Copy authzid
		p->authzid = strndup(message, (size_t) (pos - message));
		message = pos + 1;
	}
	else if (message[0] == ',')
		message++;
	else
		// Malformed GS2 header
		goto fail;

	if (! sasl_scramsha_attrlist_parse(message, &input))
		// Malformed SCRAM attribute list
		goto fail;

	if (input['m'] || ! (input['n'] && *input['n'] && input['r'] && *input['r']))
		// Unacceptable SCRAM attributes (or lack thereof)
		goto fail;

	/*
	 * TODO: Normalise the username
	 */
	if (! (s->mu = myuser_find(input['n'])))
		// No such user
		goto fail;

	if (! (s->mu->flags & MU_CRYPTPASS))
		// User's password is not encrypted
		goto fail;

	if (! sasl_scramsha_ex(s->mu->pass, &s->db))
		// User's password is not in a PBKDF2 format
		goto fail;

	if (s->db.a != prf)
		// PBKDF2 PRF algorithm in user's password hash does not match the PRF in the SCRAM mechanism name
		goto fail;

	p->username = strdup(input['n']);
	s->c_gs2_len = (size_t) (message - header);
	s->c_gs2_buf = strndup(header, s->c_gs2_len);
	s->c_msg_len = len;
	s->c_msg_buf = strndup(message, len);
	s->cn = strdup(input['r']);
	s->sn = random_string(NONCE_LENGTH);

	if (! p->username || ! s->c_gs2_buf || ! s->c_msg_buf || ! s->cn || ! s->sn)
		// Memory allocation failure
		goto fail;

	if (! (*out = mowgli_alloc(RESPONSE_LENGTH)))
		// Memory allocation failure
		goto fail;

	// Base64-encode our salt
	char Salt64[PBKDF2_SALTLEN_MAX * 3];
	if (base64_encode(s->db.salt, strlen(s->db.salt), Salt64, sizeof Salt64) == (size_t) -1)
		// Base64 encoding error
		goto fail;

	// Construct server-first-message
	const int ol = snprintf(*out, RESPONSE_LENGTH, "r=%s%s,s=%s,i=%u", s->cn, s->sn, Salt64, s->db.c);

	if (ol <= (NONCE_LENGTH + PBKDF2_SALTLEN_MIN + 16) || ol >= RESPONSE_LENGTH)
		// String writing error
		goto fail;

	*out_len = (size_t) ol;

	s->s_msg_len = *out_len;
	s->s_msg_buf = strdup(*out);

	(void) sasl_scramsha_attrlist_free(&input);
	s->step = SCRAMSHA_STEP_CLIENTPROOF;
	return ASASL_MORE;

fail:
	(void) sasl_scramsha_attrlist_free(&input);
	s->step = SCRAMSHA_STEP_FAILED;
	return ASASL_FAIL;
}

static int
sasl_scramsha_step_clientproof(sasl_session_t *const restrict p, char *const restrict data, const size_t len,
                               char **restrict out, size_t *const restrict out_len)
{
	unsigned char ClientSignature[EVP_MAX_MD_SIZE];
	unsigned char ServerSignature[EVP_MAX_MD_SIZE];
	unsigned char ClientProof[EVP_MAX_MD_SIZE];
	unsigned char ClientKey[EVP_MAX_MD_SIZE];
	unsigned char StoredKey[EVP_MAX_MD_SIZE];

	char ServerSignature64[EVP_MAX_MD_SIZE * 3];
	char AuthMessage[RESPONSE_LENGTH * 4];
	char x_nonce[RESPONSE_LENGTH * 2];
	char c_gs2_buf[RESPONSE_LENGTH];

	const unsigned char *const AuthMessageR = (const unsigned char *) AuthMessage;
	const char *const ServerSignatureT = (const char *) ServerSignature;

	struct scramsha_session *const s = p->mechdata;

	scram_attr_list input;
	(void) memset(input, 0x00, sizeof input);

	if (strnlen(data, len) != len)
		// NULL byte in data received from client
		goto fail;

	if (! sasl_scramsha_attrlist_parse(data, &input))
		// Malformed SCRAM attribute list
		goto fail;

	if (input['m'] || ! (input['c'] && *input['c'] && input['p'] && *input['p'] && input['r'] && *input['r']))
		// Unacceptable SCRAM attributes (or lack thereof)
		goto fail;

	// Concatenate the s-nonce to the c-nonce
	const int xl = snprintf(x_nonce, RESPONSE_LENGTH, "%s%s", s->cn, s->sn);

	if (xl < NONCE_LENGTH || xl >= RESPONSE_LENGTH)
		// String writing error
		goto fail;

	if (strcmp(x_nonce, input['r']) != 0)
		// Nonce from client doesn't match nonce we sent
		goto fail;

	// Decode GS2 header from client-final-message
	const size_t c_gs2_len = base64_decode(input['c'], c_gs2_buf, sizeof c_gs2_buf);

	if (c_gs2_len != s->c_gs2_len || memcmp(s->c_gs2_buf, c_gs2_buf, c_gs2_len) != 0)
		// GS2 header from client's second message doesn't match their first message
		goto fail;

	// Decode ClientProof from client-final-message
	if (base64_decode(input['p'], (char *) ClientProof, sizeof ClientProof) != s->db.dl)
		// Base64 decoding error
		goto fail;

	// Construct AuthMessage
	const int alen = snprintf(AuthMessage, sizeof AuthMessage, "%s,%s,c=%s,r=%s",
	                          s->c_msg_buf, s->s_msg_buf, input['c'], input['r']);

	if (alen < NONCE_LENGTH || alen >= (int) sizeof AuthMessage)
		// String writing error
		goto fail;

	// Calculate ClientSignature
	if (! HMAC(s->db.md, s->db.shk, (int) s->db.dl, AuthMessageR, (size_t) alen, ClientSignature, NULL))
		// HMAC calculation error
		goto fail;

	// XOR ClientProof with calculated ClientSignature to derive ClientKey
	for (size_t x = 0; x < s->db.dl; x++)
		ClientKey[x] = ClientProof[x] ^ ClientSignature[x];

	// Compute StoredKey from derived ClientKey
	if (EVP_Digest(ClientKey, s->db.dl, StoredKey, NULL, s->db.md, NULL) != 1)
		// Digest calculation error
		goto fail;

	// Check computed StoredKey matches the database StoredKey
	if (memcmp(StoredKey, s->db.shk, s->db.dl) != 0)
		// Incorrect password?
		goto fail;

	/* ******************************************************** *
	 * AUTHENTICATION OF THE CLIENT HAS SUCCEEDED AT THIS POINT *
	 * ******************************************************** */

	// Calculate ServerSignature
	if (! HMAC(s->db.md, s->db.ssk, (int) s->db.dl, AuthMessageR, (size_t) alen, ServerSignature, NULL))
		// HMAC calculation error
		goto fail;

	// Encode ServerSignature
	*out_len = base64_encode(ServerSignatureT, s->db.dl, ServerSignature64, sizeof ServerSignature64);

	if (*out_len == (size_t) -1)
		// Base64 encoding error
		goto fail;

	// We need to prepend "v=" to the Base64 data
	(*out_len) += 2;

	if (! (*out = mowgli_alloc(*out_len)))
		// Memory allocation failure
		goto fail;

	// Create server-final-message
	(*out)[0] = 'v';
	(*out)[1] = '=';
	(void) memcpy((*out) + 2, ServerSignature64, *out_len - 2);

	(void) sasl_scramsha_attrlist_free(&input);
	s->step = SCRAMSHA_STEP_PASSED;
	return ASASL_MORE;

fail:
	(void) sasl_scramsha_attrlist_free(&input);
	s->step = SCRAMSHA_STEP_FAILED;
	return ASASL_FAIL;
}

static int
sasl_scramsha_step_success(sasl_session_t *const restrict p)
{
	struct scramsha_session *const s = p->mechdata;

	if (s->db.scram)
		// User's password hash was already in SCRAM format, nothing to do
		return ASASL_DONE;

	/*
	 * An SASL SCRAM-SHA login has succeeded, but the user's database hash was not in SCRAM format.
	 *
	 * If the database is breached in the future, the raw PBKDF2 digest ("SaltedPassword" in RFC 5802)
	 * can be used to compute the ClientKey and impersonate the client to this service.
	 *
	 * Convert the SaltedPassword into a ServerKey and StoredKey now, and then write those back to the
	 * database, overwriting SaltedPassword. Verification of plaintext passwords can still take place
	 * (e.g. for SASL PLAIN or NickServ IDENTIFY) because modules/crypto/pbkdf2v2 can compute ServerKey
	 * from the provided plaintext password and compare it to the stored ServerKey.
	 */

	(void) slog(LG_INFO, "%s: login succeeded, attempting to convert user's hash to SCRAM format", __func__);

	char csk64[EVP_MAX_MD_SIZE];
	char chk64[EVP_MAX_MD_SIZE];
	char res[PASSLEN];

	if (base64_encode((const char *) s->db.ssk, s->db.dl, csk64, sizeof csk64) == (size_t) -1)
	{
		(void) slog(LG_ERROR, "%s: base64_encode for ssk failed", __func__);
	}
	else if (base64_encode((const char *) s->db.shk, s->db.dl, chk64, sizeof chk64) == (size_t) -1)
	{
		(void) slog(LG_ERROR, "%s: base64_encode for shk failed", __func__);
	}
	else if (snprintf(res, PASSLEN, PBKDF2_FS_SAVEHASH, s->db.a, s->db.c, s->db.salt, csk64, chk64) >= PASSLEN)
	{
		(void) slog(LG_ERROR, "%s: snprintf(3) would have overflowed result buffer (BUG)", __func__);
	}
	else if (strlen(res) < (sizeof(PBKDF2_FS_SAVEHASH) + PBKDF2_SALTLEN_MIN + (2 * s->db.dl)))
	{
		(void) slog(LG_ERROR, "%s: snprintf(3) didn't write enough data (BUG?)", __func__);
	}
	else
	{
		(void) mowgli_strlcpy(s->mu->pass, res, PASSLEN);
		(void) slog(LG_DEBUG, "%s: succeeded", __func__);
	}

	return ASASL_DONE;
}

static inline int
sasl_scramsha_step_x(sasl_session_t *const restrict p, char *const restrict message, const size_t len,
                     char **restrict out, size_t *const restrict out_len, const unsigned int prf)
{
	const struct scramsha_session *const s = p->mechdata;

	switch (s->step)
	{
		case SCRAMSHA_STEP_CLIENTFIRST:
			return sasl_scramsha_step_clientfirst(p, message, len, out, out_len, prf);

		case SCRAMSHA_STEP_CLIENTPROOF:
			return sasl_scramsha_step_clientproof(p, message, len, out, out_len);

		case SCRAMSHA_STEP_PASSED:
			return sasl_scramsha_step_success(p);

		case SCRAMSHA_STEP_FAILED:
			return ASASL_FAIL;
	}
}

static int
sasl_scramsha_step_sha1(sasl_session_t *const restrict p, char *const restrict message, const size_t len,
                        char **restrict out, size_t *const restrict out_len)
{
	return sasl_scramsha_step_x(p, message, len, out, out_len, PBKDF2_PRF_SCRAM_SHA1);
}

static int
sasl_scramsha_step_sha2_256(sasl_session_t *const restrict p, char *const restrict message, const size_t len,
                            char **restrict out, size_t *const restrict out_len)
{
	return sasl_scramsha_step_x(p, message, len, out, out_len, PBKDF2_PRF_SCRAM_SHA2_256);
}

static sasl_mechanism_t sasl_scramsha_mech_sha1 = {

	"SCRAM-SHA-1", &sasl_scramsha_start, &sasl_scramsha_step_sha1, &sasl_scramsha_finish,
};

static sasl_mechanism_t sasl_scramsha_mech_sha2_256 = {

	"SCRAM-SHA-256", &sasl_scramsha_start, &sasl_scramsha_step_sha2_256, &sasl_scramsha_finish,
};

static const sasl_mech_register_func_t *regfuncs = NULL;

static void
sasl_scramsha_config_ready(void __attribute__((unused)) *const restrict unused)
{
	(void) regfuncs->mech_unregister(&sasl_scramsha_mech_sha1);
	(void) regfuncs->mech_unregister(&sasl_scramsha_mech_sha2_256);

	const unsigned int *const pbkdf2v2_digest = module_locate_symbol("crypto/pbkdf2v2", "pbkdf2v2_digest");

	if (! pbkdf2v2_digest)
		// module_locate_symbol() logs error messages on failure
		return;

	switch (*pbkdf2v2_digest)
	{
		case PBKDF2_PRF_SCRAM_SHA1:
			(void) regfuncs->mech_register(&sasl_scramsha_mech_sha1);
			return;

		case PBKDF2_PRF_SCRAM_SHA2_256:
			(void) regfuncs->mech_register(&sasl_scramsha_mech_sha2_256);
			return;
	}

	(void) slog(LG_ERROR, "%s: pbkdf2v2::digest is not set to a supported value -- "
	                      "this module will not do anything", __func__);
}

static void
sasl_scramsha_modinit(module_t *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, sasl_scramsha_ex, "crypto/pbkdf2v2", "atheme_pbkdf2v2_scram_ex");
	MODULE_TRY_REQUEST_SYMBOL(m, regfuncs, "saslserv/main", "sasl_mech_register_funcs");

	(void) hook_add_event("config_ready");
	(void) hook_add_config_ready(sasl_scramsha_config_ready);
}

static void
sasl_scramsha_moddeinit(const module_unload_intent_t __attribute__((unused)) intent)
{
	(void) regfuncs->mech_unregister(&sasl_scramsha_mech_sha1);
	(void) regfuncs->mech_unregister(&sasl_scramsha_mech_sha2_256);

	(void) hook_del_config_ready(sasl_scramsha_config_ready);
}

DECLARE_MODULE_V1("saslserv/scram-sha", false, sasl_scramsha_modinit, sasl_scramsha_moddeinit,
                  PACKAGE_STRING, VENDOR_STRING);

#endif /* HAVE_OPENSSL */
