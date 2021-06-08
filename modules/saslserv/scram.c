/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2014 Mantas Mikulėnas <grawity@gmail.com>
 * Copyright (C) 2017-2019 Aaron M. D. Jones <me@aaronmdjones.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * SCRAM-SHA-1, SCRAM-SHA-256 & SCRAM-SHA-512 SASL mechanisms provider.
 *
 * See the following RFCs for details:
 *
 *   - RFC 5802 <https://tools.ietf.org/html/rfc5802>
 *     "Salted Challenge Response Authentication Mechanism (SCRAM)"
 *
 *   - RFC 7677 <https://tools.ietf.org/html/rfc7677>
 *     "SCRAM-SHA-256 and SCRAM-SHA-256-PLUS SASL Mechanisms"
 */

#include <atheme.h>

#ifdef HAVE_LIBIDN

#define SCRAM_MDLEN_MIN                 DIGEST_MDLEN_SHA1
#define SCRAM_MDLEN_MAX                 DIGEST_MDLEN_SHA2_512

#define SCRAM_NONCE_LENGTH              64U         // This should be more than sufficient
#define SCRAM_NONCE_LENGTH_MIN          8U          // Minimum acceptable client nonce length
#define SCRAM_NONCE_LENGTH_MAX          512U        // Maximum acceptable client nonce length
#define SCRAM_NONCE_LENGTH_MIN_COMBINED (SCRAM_NONCE_LENGTH + SCRAM_NONCE_LENGTH_MIN)
#define SCRAM_NONCE_LENGTH_MAX_COMBINED (SCRAM_NONCE_LENGTH + SCRAM_NONCE_LENGTH_MAX)

// strlen("n,a=,") == 5U
#define SCRAM_GS2HDR_LENGTH_MAX         (5U + NICKLEN)

// strlen("10000") == 5U
// strlen("5000000") == 7U
// strlen("r=,s=,i=") == 8U
#define SCRAM_RESPONSE1_LENGTH_MIN      (SCRAM_NONCE_LENGTH_MIN_COMBINED + \
                                         BASE64_SIZE_RAW(PBKDF2_SALTLEN_MIN) + \
                                         5U + 8U)
#define SCRAM_RESPONSE1_LENGTH_MAX      (SCRAM_NONCE_LENGTH_MAX_COMBINED + \
                                         BASE64_SIZE_RAW(PBKDF2_SALTLEN_MAX) + \
                                         7U + 8U)

// strlen("v=") == 2U
#define SCRAM_RESPONSE2_LENGTH_MAX      (BASE64_SIZE_RAW(SCRAM_MDLEN_MAX) + 2U)

// strlen("10000") == 5U
// strlen("5000000") == 7U
// strlen("$z$64$$$$") == 9U
#define SCRAM_PASSHASH_LENGTH_MIN       (5U + BASE64_SIZE_RAW(PBKDF2_SALTLEN_MIN) + \
                                         (2U * BASE64_SIZE_RAW(SCRAM_MDLEN_MIN)) + 9U)
#define SCRAM_PASSHASH_LENGTH_MAX       (7U + BASE64_SIZE_RAW(PBKDF2_SALTLEN_MAX) + \
                                         (2U * BASE64_SIZE_RAW(SCRAM_MDLEN_MAX)) + 9U)

struct scram_attribute
{
	char *                      value;
	size_t                      length;
};

struct scram_session
{
	mowgli_node_t               node;       // For entry into mowgli_list_t scram_sessions
	struct pbkdf2v2_dbentry     db;         // Parsed credentials from database
	struct myuser              *mu;         // User we are authenticating as
	char                       *c_msg_buf;  // Client's first message
	size_t                      c_gs2_len;  // Client's GS2 header (base64-encoded length)
	size_t                      c_msg_len;  // Client's first message (length)
	size_t                      s_msg_len;  // Server's first message (length)
	size_t                      nonce_len;  // Length of combined (client + server) nonce below
	bool                        complete;   // Authentication is complete, waiting for client to confirm

	// Client's GS2 header (base64-encoded)
	char                        c_gs2_b64[BASE64_SIZE_STR(SCRAM_GS2HDR_LENGTH_MAX)];

	// Server's first message
	char                        s_msg_buf[SCRAM_RESPONSE1_LENGTH_MAX + 1];

	// Combined (client + server) nonce
	char                        nonce[SCRAM_NONCE_LENGTH_MAX_COMBINED + 1];
};

typedef struct scram_attribute scram_attr_list[128];

static mowgli_list_t scram_sessions;

static const struct sasl_core_functions *sasl_core_functions = NULL;
static const struct pbkdf2v2_scram_functions *pbkdf2v2_scram_functions = NULL;

static void
scram_myuser_delete(struct myuser *const restrict mu)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, scram_sessions.head)
	{
		struct scram_session *const s = n->data;

		if (s->mu == mu)
			s->mu = NULL;
	}
}

static bool ATHEME_FATTR_WUR
scram_attrlist_parse(const char *restrict str, const char *const restrict end,
                     scram_attr_list *const restrict attributes)
{
	(void) memset(*attributes, 0x00, sizeof *attributes);

	for (;;)
	{
		unsigned char name = (unsigned char) *str++;

		if (name < 'A' || (name > 'Z' && name < 'a') || name > 'z')
		{
			// RFC 5802 Section 5: "All attribute names are single US-ASCII letters"
			(void) slog(LG_DEBUG, "%s: invalid attribute name", MOWGLI_FUNC_NAME);
			return false;
		}

		if ((*attributes)[name].value)
		{
			(void) slog(LG_DEBUG, "%s: duplicated attribute '%c'", MOWGLI_FUNC_NAME, (char) name);
			return false;
		}

		if (*str++ == '=' && (*str != ',' && *str != 0x00))
		{
			const char *const pos = strchr(str + 1, ',');
			const char *const idx = ((pos != NULL) ? pos : end);

			(*attributes)[name].length = (size_t) (idx - str);
			(*attributes)[name].value = sstrndup(str, (*attributes)[name].length);

			(void) slog(LG_DEBUG, "%s: parsed '%c'='%s'", MOWGLI_FUNC_NAME,
			                      (char) name, (*attributes)[name].value);

			if (pos)
				str = pos + 1;
			else
				// Reached end of attribute list
				return true;
		}
		else
		{
			(void) slog(LG_DEBUG, "%s: attribute '%c' without value", MOWGLI_FUNC_NAME, (char) name);
			return false;
		}
	}
}

static inline void
scram_attrlist_free(scram_attr_list *const restrict attributes)
{
	for (unsigned char x = 'A'; x <= 'z'; x++)
	{
		if (! (*attributes)[x].value)
			continue;

		(void) smemzerofree((*attributes)[x].value, (*attributes)[x].length);

		(*attributes)[x].value = NULL;
		(*attributes)[x].length = 0;
	}
}

static const char **
scram_get_avoidance_mechlist(const unsigned int db_prf)
{
	switch (db_prf)
	{
		case PBKDF2_PRF_SCRAM_SHA1_S64:
		{
			static const char *avoid[] = { "SCRAM-SHA-256", "SCRAM-SHA-512", NULL };
			return avoid;
		}

		case PBKDF2_PRF_SCRAM_SHA2_256_S64:
		{
			static const char *avoid[] = { "SCRAM-SHA-1", "SCRAM-SHA-512", NULL };
			return avoid;
		}

		case PBKDF2_PRF_SCRAM_SHA2_512_S64:
		{
			static const char *avoid[] = { "SCRAM-SHA-1", "SCRAM-SHA-256", NULL };
			return avoid;
		}

		default:
			break;
	}

	return NULL;
}

static void
scram_error(const char *const restrict errtext, struct sasl_output_buf *const restrict out)
{
	static char errbuf[BUFSIZE];

	const int errlen = snprintf(errbuf, sizeof errbuf, "e=%s", errtext);

	if (! (errlen <= 2 || errlen >= (int) sizeof errbuf))
	{
		out->buf = errbuf;
		out->len = (size_t) errlen;
	}
}

static enum sasl_mechanism_result ATHEME_FATTR_WUR
scram_step_clientfirst(struct sasl_session *const restrict p, const struct sasl_input_buf *const restrict in,
                       struct sasl_output_buf *const restrict out, const unsigned int prf)
{
	if (! (in && in->buf && in->len))
	{
		(void) slog(LG_DEBUG, "%s: no data received from client", MOWGLI_FUNC_NAME);
		(void) scram_error("other-error", out);
		return ASASL_MRESULT_ERROR;
	}
	if (memchr(in->buf, 0x00, in->len))
	{
		(void) slog(LG_DEBUG, "%s: NULL byte in data received from client", MOWGLI_FUNC_NAME);
		(void) scram_error("other-error", out);
		return ASASL_MRESULT_ERROR;
	}

	const char *const header = in->buf;
	const char *message = in->buf;

	enum sasl_mechanism_result retval = ASASL_MRESULT_CONTINUE;
	char c_gs2_b64[BASE64_SIZE_STR(SCRAM_GS2HDR_LENGTH_MAX)];
	char authzid[NICKLEN + 1];
	char authcid[NICKLEN + 1];
	struct myuser *mu = NULL;

	scram_attr_list attributes;
	struct pbkdf2v2_dbentry db;

	switch (*message++)
	{
		// RFC 5802 Section 7 (gs2-cbind-flag)
		case 'y':
		case 'n':
			break;

		case 'p':
			(void) slog(LG_DEBUG, "%s: channel binding requested but unsupported", MOWGLI_FUNC_NAME);
			(void) scram_error("channel-binding-not-supported", out);
			return ASASL_MRESULT_ERROR;

		default:
			(void) slog(LG_DEBUG, "%s: malformed GS2 header (invalid first byte)", MOWGLI_FUNC_NAME);
			(void) scram_error("other-error", out);
			return ASASL_MRESULT_ERROR;
	}
	if (*message++ != ',')
	{
		(void) slog(LG_DEBUG, "%s: malformed GS2 header (cbind flag not one letter)", MOWGLI_FUNC_NAME);
		(void) scram_error("other-error", out);
		return ASASL_MRESULT_ERROR;
	}

	// Does GS2 header include an authzid ?
	if (message[0] == 'a' && message[1] == '=')
	{
		message += 2;

		// Locate its end
		const char *const pos = strchr(message + 1, ',');
		if (! pos)
		{
			(void) slog(LG_DEBUG, "%s: malformed GS2 header (no end to authzid)", MOWGLI_FUNC_NAME);
			(void) scram_error("other-error", out);
			return ASASL_MRESULT_ERROR;
		}

		// Check its length
		const size_t authzidLen = (size_t) (pos - message);
		if (authzidLen > NICKLEN)
		{
			(void) slog(LG_DEBUG, "%s: unacceptable authzid length '%zu'",
			                      MOWGLI_FUNC_NAME, authzidLen);
			(void) scram_error("authzid-too-long", out);
			return ASASL_MRESULT_ERROR;
		}

		// Copy it
		(void) memset(authzid, 0x00, sizeof authzid);
		(void) memcpy(authzid, message, authzidLen);

		// Normalize it
		if (! pbkdf2v2_scram_functions->normalize(authzid, sizeof authzid))
		{
			(void) slog(LG_DEBUG, "%s: SASLprep normalization of authzid failed", MOWGLI_FUNC_NAME);
			(void) scram_error("invalid-username-encoding", out);
			return ASASL_MRESULT_ERROR;
		}

		// Log it
		(void) slog(LG_DEBUG, "%s: parsed authzid '%s'", MOWGLI_FUNC_NAME, authzid);

		// Check it exists and can login
		if (! sasl_core_functions->authzid_can_login(p, authzid, NULL))
		{
			(void) slog(LG_DEBUG, "%s: authzid_can_login failed", MOWGLI_FUNC_NAME);
			(void) scram_error("other-error", out);
			return ASASL_MRESULT_ERROR;
		}

		message = pos + 1;
	}
	else if (*message++ != ',')
	{
		(void) slog(LG_DEBUG, "%s: malformed GS2 header (authzid section not empty)", MOWGLI_FUNC_NAME);
		(void) scram_error("other-error", out);
		return ASASL_MRESULT_ERROR;
	}

	const size_t header_len = (size_t) (message - header);
	const size_t c_gs2_len = base64_encode(header, header_len, c_gs2_b64, sizeof c_gs2_b64);

	if (c_gs2_len != BASE64_SIZE_RAW(header_len))
	{
		(void) slog(LG_ERROR, "%s: base64_encode() for GS2 header failed (BUG?)", MOWGLI_FUNC_NAME);
		(void) scram_error("other-error", out);
		return ASASL_MRESULT_ERROR;
	}
	if (! scram_attrlist_parse(message, ((const char *) in->buf) + in->len, &attributes))
	{
		// Malformed SCRAM attribute list
		(void) scram_error("other-error", out);
		goto error;
	}
	if (attributes['m'].value)
	{
		(void) slog(LG_DEBUG, "%s: extensions are not supported", MOWGLI_FUNC_NAME);
		(void) scram_error("extensions-not-supported", out);
		goto error;
	}
	if (! (attributes['n'].value && attributes['r'].value))
	{
		(void) slog(LG_DEBUG, "%s: required attribute missing", MOWGLI_FUNC_NAME);
		(void) scram_error("other-error", out);
		goto error;
	}
	if (attributes['r'].length < SCRAM_NONCE_LENGTH_MIN || attributes['r'].length > SCRAM_NONCE_LENGTH_MAX)
	{
		(void) slog(LG_DEBUG, "%s: nonce length '%zu' unacceptable", MOWGLI_FUNC_NAME, attributes['r'].length);
		(void) scram_error("nonce-unacceptable", out);
		goto error;
	}
	for (size_t x = 0; x < attributes['r'].length; x++)
	{
		const unsigned char nchar = (unsigned char) attributes['r'].value[x];

		if (nchar <= 0x20U || nchar == 0x2CU || nchar >= 0x7FU)
		{
			(void) slog(LG_DEBUG, "%s: nonce is not entirely RFC5802 'printable'", MOWGLI_FUNC_NAME);
			(void) scram_error("nonce-unacceptable", out);
			goto error;
		}
	}
	if (attributes['n'].length > NICKLEN)
	{
		(void) slog(LG_DEBUG, "%s: unacceptable authcid length '%zu'", MOWGLI_FUNC_NAME, attributes['n'].length);
		(void) scram_error("authcid-too-long", out);
		goto error;
	}

	(void) memset(authcid, 0x00, sizeof authcid);
	(void) memcpy(authcid, attributes['n'].value, attributes['n'].length);

	if (! pbkdf2v2_scram_functions->normalize(authcid, sizeof authcid))
	{
		(void) slog(LG_DEBUG, "%s: SASLprep normalization of authcid failed", MOWGLI_FUNC_NAME);
		(void) scram_error("invalid-username-encoding", out);
		goto error;
	}

	(void) slog(LG_DEBUG, "%s: parsed authcid '%s'", MOWGLI_FUNC_NAME, authcid);

	if (! sasl_core_functions->authcid_can_login(p, authcid, &mu))
	{
		(void) slog(LG_DEBUG, "%s: authcid_can_login failed", MOWGLI_FUNC_NAME);
		(void) scram_error("other-error", out);
		goto error;
	}
	if (! mu)
	{
		(void) slog(LG_ERROR, "%s: authcid_can_login did not set 'mu' (BUG)", MOWGLI_FUNC_NAME);
		(void) scram_error("other-error", out);
		goto error;
	}
	if (! (mu->flags & MU_CRYPTPASS))
	{
		(void) slog(LG_DEBUG, "%s: user's password is not encrypted", MOWGLI_FUNC_NAME);
		(void) scram_error("other-error", out);
		goto error;
	}
	if (! pbkdf2v2_scram_functions->dbextract(mu->pass, &db))
	{
		// User's password hash is not in a compatible (PBKDF2 v2) format
		(void) scram_error("other-error", out);
		goto error;
	}
	if (db.a != prf)
	{
		(void) slog(LG_DEBUG, "%s: PRF ID mismatch: server(%u) != client(%u)", MOWGLI_FUNC_NAME, db.a, prf);
		(void) sasl_core_functions->recalc_mechlist(p, mu, scram_get_avoidance_mechlist(db.a));
		(void) scram_error("digest-algorithm-mismatch", out);
		goto error;
	}

	// These cannot fail
	struct scram_session *const s = smalloc(sizeof *s);

	s->nonce_len = attributes['r'].length + SCRAM_NONCE_LENGTH;
	s->c_gs2_len = c_gs2_len;
	s->c_msg_len = (in->len - header_len);
	s->c_msg_buf = sstrndup(message, s->c_msg_len);
	s->mu = mu;

	(void) mowgli_node_add(s, &s->node, &scram_sessions);
	(void) memcpy(&s->db, &db, sizeof db);
	(void) memcpy(s->c_gs2_b64, c_gs2_b64, c_gs2_len);
	(void) memcpy(s->nonce, attributes['r'].value, attributes['r'].length);
	(void) atheme_random_str(s->nonce + attributes['r'].length, SCRAM_NONCE_LENGTH);

	p->mechdata = s;

	// Construct server-first-message
	const int respLen = snprintf(s->s_msg_buf, sizeof s->s_msg_buf, "r=%s,s=%s,i=%u",
	                             s->nonce, s->db.salt64, s->db.c);

	if (respLen < (int) SCRAM_RESPONSE1_LENGTH_MIN || respLen > (int) SCRAM_RESPONSE1_LENGTH_MAX)
	{
		(void) slog(LG_ERROR, "%s: snprintf(3) for server-first-message failed (BUG)", MOWGLI_FUNC_NAME);
		(void) scram_error("other-error", out);
		goto error;
	}

	s->s_msg_len = (size_t) respLen;

	out->buf = s->s_msg_buf;
	out->len = s->s_msg_len;

	goto cleanup;

error:
	retval = ASASL_MRESULT_ERROR;
	goto cleanup;

cleanup:
	(void) smemzero(&db, sizeof db);
	(void) scram_attrlist_free(&attributes);
	return retval;
}

static enum sasl_mechanism_result ATHEME_FATTR_WUR
scram_step_clientproof(struct sasl_session *const restrict p, const struct sasl_input_buf *const restrict in,
                       struct sasl_output_buf *const restrict out)
{
	if (! (in && in->buf && in->len))
	{
		(void) slog(LG_DEBUG, "%s: no data received from client", MOWGLI_FUNC_NAME);
		(void) scram_error("other-error", out);
		return ASASL_MRESULT_ERROR;
	}

	// This buffer contains sensitive information
	*(in->flags) |= ASASL_INFLAG_WIPE_BUF;

	if (memchr(in->buf, 0x00, in->len))
	{
		(void) slog(LG_DEBUG, "%s: NULL byte in data received from client", MOWGLI_FUNC_NAME);
		(void) scram_error("other-error", out);
		return ASASL_MRESULT_ERROR;
	}

	enum sasl_mechanism_result retval = ASASL_MRESULT_CONTINUE;
	struct scram_session *const s = p->mechdata;
	static char response[SCRAM_RESPONSE2_LENGTH_MAX + 1];

	// User account was dropped during negotiation
	if (! s->mu)
	{
		(void) slog(LG_DEBUG, "%s: user account dropped during negotiation", MOWGLI_FUNC_NAME);
		(void) scram_error("other-error", out);
		return ASASL_MRESULT_ERROR;
	}

	scram_attr_list attributes;
	unsigned char ClientProof[SCRAM_MDLEN_MAX];
	unsigned char ClientKey[SCRAM_MDLEN_MAX];
	unsigned char StoredKey[SCRAM_MDLEN_MAX];
	unsigned char ClientSig[SCRAM_MDLEN_MAX];
	unsigned char ServerSig[SCRAM_MDLEN_MAX];
	char ServerSig64[BASE64_SIZE_STR(SCRAM_MDLEN_MAX)];

	if (! scram_attrlist_parse(in->buf, ((const char *) in->buf) + in->len, &attributes))
	{
		// Malformed SCRAM attribute list
		(void) scram_error("other-error", out);
		goto error;
	}
	if (attributes['m'].value)
	{
		(void) slog(LG_DEBUG, "%s: extensions are not supported", MOWGLI_FUNC_NAME);
		(void) scram_error("extensions-not-supported", out);
		goto error;
	}
	if (! (attributes['c'].value && attributes['p'].value && attributes['r'].value))
	{
		(void) slog(LG_DEBUG, "%s: required attribute missing", MOWGLI_FUNC_NAME);
		(void) scram_error("other-error", out);
		goto error;
	}

	// Verify nonce received matches nonce sent
	if (strcmp(s->nonce, attributes['r'].value) != 0)
	{
		(void) slog(LG_DEBUG, "%s: nonce sent by client doesn't match nonce we sent", MOWGLI_FUNC_NAME);
		(void) scram_error("nonce-unacceptable", out);
		goto error;
	}

	// Verify GS2 header from client-final-message against the original header received above
	if (strcmp(s->c_gs2_b64, attributes['c'].value) != 0)
	{
		(void) slog(LG_DEBUG, "%s: GS2 header mismatch", MOWGLI_FUNC_NAME);
		(void) scram_error("other-error", out);
		goto error;
	}

	// Decode ClientProof from client-final-message
	if (base64_decode(attributes['p'].value, ClientProof, sizeof ClientProof) != s->db.dl)
	{
		(void) slog(LG_DEBUG, "%s: base64_decode() for ClientProof failed", MOWGLI_FUNC_NAME);
		(void) scram_error("other-error", out);
		goto error;
	}

	// Construct AuthMessage
	const struct digest_vector AuthMessage[] = {
		{ s->c_msg_buf, s->c_msg_len },
		{          ",", 1            },
		{ s->s_msg_buf, s->s_msg_len },
		{        ",c=", 3            },
		{ s->c_gs2_b64, s->c_gs2_len },
		{        ",r=", 3            },
		{     s->nonce, s->nonce_len },
	};
	const size_t AuthMsgLen = (sizeof AuthMessage) / (sizeof AuthMessage[0]);

	// Calculate ClientSignature
	if (! digest_oneshot_hmac_vector(s->db.md, s->db.shk, s->db.dl, AuthMessage, AuthMsgLen, ClientSig, NULL))
	{
		(void) slog(LG_ERROR, "%s: digest_oneshot_hmac() for ClientSignature failed (BUG)", MOWGLI_FUNC_NAME);
		(void) scram_error("other-error", out);
		goto error;
	}

	// XOR ClientProof with calculated ClientSignature to derive ClientKey
	for (size_t x = 0; x < s->db.dl; x++)
		ClientKey[x] = ClientProof[x] ^ ClientSig[x];

	// Compute StoredKey from derived ClientKey
	if (! digest_oneshot(s->db.md, ClientKey, s->db.dl, StoredKey, NULL))
	{
		(void) slog(LG_ERROR, "%s: digest_oneshot() for StoredKey failed (BUG)", MOWGLI_FUNC_NAME);
		(void) scram_error("other-error", out);
		goto error;
	}

	// Check computed StoredKey matches the database StoredKey
	if (smemcmp(StoredKey, s->db.shk, s->db.dl) != 0)
	{
		(void) slog(LG_DEBUG, "%s: smemcmp() mismatch on StoredKey; incorrect password?", MOWGLI_FUNC_NAME);
		(void) scram_error("invalid-proof", out);
		goto fail;
	}

	/* ******************************************************** *
	 * AUTHENTICATION OF THE CLIENT HAS SUCCEEDED AT THIS POINT *
	 * ******************************************************** */
	(void) slog(LG_DEBUG, "%s: authentication successful", MOWGLI_FUNC_NAME);

	// Calculate ServerSignature
	if (! digest_oneshot_hmac_vector(s->db.md, s->db.ssk, s->db.dl, AuthMessage, AuthMsgLen, ServerSig, NULL))
	{
		(void) slog(LG_ERROR, "%s: digest_oneshot_hmac() for ServerSignature failed (BUG)", MOWGLI_FUNC_NAME);
		(void) scram_error("other-error", out);
		goto error;
	}

	// Encode ServerSignature
	const size_t ServerSig64Len = base64_encode(ServerSig, s->db.dl, ServerSig64, sizeof ServerSig64);
	if (ServerSig64Len != BASE64_SIZE_RAW(s->db.dl))
	{
		(void) slog(LG_ERROR, "%s: base64_encode() for ServerSignature failed (BUG)", MOWGLI_FUNC_NAME);
		(void) scram_error("other-error", out);
		goto error;
	}

	// Construct server-final-message
	const int respLen = snprintf(response, sizeof response, "v=%s", ServerSig64);
	if (respLen != (int) (2U + ServerSig64Len))
	{
		(void) slog(LG_ERROR, "%s: snprintf(3) did not write an acceptable amount of data (%d) (BUG)",
		                      MOWGLI_FUNC_NAME, respLen);
		(void) scram_error("other-error", out);
		goto error;
	}

	// This buffer contains sensitive information
	out->flags |= ASASL_OUTFLAG_WIPE_BUF;
	out->buf = response;
	out->len = (size_t) respLen;

	s->complete = true;

	goto cleanup;

error:
	retval = ASASL_MRESULT_ERROR;
	goto cleanup;

fail:
	retval = ASASL_MRESULT_FAILURE;
	goto cleanup;

cleanup:
	(void) smemzero(ClientProof, sizeof ClientProof);
	(void) smemzero(ClientKey, sizeof ClientKey);
	(void) smemzero(StoredKey, sizeof StoredKey);
	(void) smemzero(ClientSig, sizeof ClientSig);
	(void) smemzero(ServerSig, sizeof ServerSig);
	(void) smemzero(ServerSig64, sizeof ServerSig64);
	(void) scram_attrlist_free(&attributes);
	return retval;
}

static enum sasl_mechanism_result ATHEME_FATTR_WUR
scram_step_success(struct sasl_session *const restrict p)
{
	const struct scram_session *const s = p->mechdata;

	// User account was dropped during negotiation
	if (! s->mu)
	{
		(void) slog(LG_DEBUG, "%s: user account dropped during negotiation", MOWGLI_FUNC_NAME);
		return ASASL_MRESULT_ERROR;
	}

	if (s->db.scram)
		// User's password hash was already in SCRAM format, nothing to do
		return ASASL_MRESULT_SUCCESS;

	/* An SASL SCRAM login has succeeded, but the user's database hash was not in SCRAM format.
	 *
	 * If the database is breached in the future, the raw PBKDF2 digest ("SaltedPassword" in RFC 5802)
	 * can be used to compute the ClientKey and impersonate the client to this service.
	 *
	 * Convert the SaltedPassword into a ServerKey and StoredKey now, and then write those back to the
	 * database, overwriting SaltedPassword. Verification of plaintext passwords can still take place
	 * (e.g. for SASL PLAIN or NickServ IDENTIFY) because modules/crypto/pbkdf2v2 can compute ServerKey
	 * from the provided plaintext password and compare it to the stored ServerKey.
	 */

	(void) slog(LG_INFO, "%s: login succeeded, attempting to convert user's hash to SCRAM format",
	                     MOWGLI_FUNC_NAME);

	char csk64[BASE64_SIZE_STR(SCRAM_MDLEN_MAX)];
	char chk64[BASE64_SIZE_STR(SCRAM_MDLEN_MAX)];

	if (base64_encode(s->db.ssk, s->db.dl, csk64, sizeof csk64) != BASE64_SIZE_RAW(s->db.dl))
	{
		(void) slog(LG_ERROR, "%s: base64_encode() for ServerKey failed (BUG)", MOWGLI_FUNC_NAME);
		goto end;
	}

	if (base64_encode(s->db.shk, s->db.dl, chk64, sizeof chk64) != BASE64_SIZE_RAW(s->db.dl))
	{
		(void) slog(LG_ERROR, "%s: base64_encode() for StoredKey failed (BUG)", MOWGLI_FUNC_NAME);
		goto end;
	}

	char buf[PASSLEN + 1];

	const int ret = snprintf(buf, sizeof buf, PBKDF2_FS_SAVEHASH, s->db.a, s->db.c, s->db.salt64, csk64, chk64);

	if (ret < (int) SCRAM_PASSHASH_LENGTH_MIN || ret > (int) SCRAM_PASSHASH_LENGTH_MAX)
	{
		(void) slog(LG_ERROR, "%s: snprintf(3) did not write an acceptable amount of data (%d) (BUG)",
		                      MOWGLI_FUNC_NAME, ret);
		goto end;
	}

	(void) slog(LG_DEBUG, "%s: succeeded", MOWGLI_FUNC_NAME);
	(void) smemzero(s->mu->pass, sizeof s->mu->pass);
	(void) memcpy(s->mu->pass, buf, (size_t) ret);
	(void) smemzero(buf, sizeof buf);
	(void) hook_call_myuser_changed_password_or_hash(s->mu);

end:
	(void) smemzero(csk64, sizeof csk64);
	(void) smemzero(chk64, sizeof chk64);
	return ASASL_MRESULT_SUCCESS;
}

static inline enum sasl_mechanism_result ATHEME_FATTR_WUR
scram_step_dispatch(struct sasl_session *const restrict p, const struct sasl_input_buf *const restrict in,
                    struct sasl_output_buf *const restrict out, const unsigned int prf)
{
	if (! p)
		return ASASL_MRESULT_ERROR;

	const struct scram_session *const s = p->mechdata;

	if (! s)
		return scram_step_clientfirst(p, in, out, prf);
	else if (! s->complete)
		return scram_step_clientproof(p, in, out);
	else
		return scram_step_success(p);
}

static enum sasl_mechanism_result ATHEME_FATTR_WUR
sasl_mech_scram_sha1_step(struct sasl_session *const restrict p, const struct sasl_input_buf *const restrict in,
                          struct sasl_output_buf *const restrict out)
{
	return scram_step_dispatch(p, in, out, PBKDF2_PRF_SCRAM_SHA1_S64);
}

static enum sasl_mechanism_result ATHEME_FATTR_WUR
sasl_mech_scram_sha2_256_step(struct sasl_session *const restrict p, const struct sasl_input_buf *const restrict in,
                              struct sasl_output_buf *const restrict out)
{
	return scram_step_dispatch(p, in, out, PBKDF2_PRF_SCRAM_SHA2_256_S64);
}

static enum sasl_mechanism_result ATHEME_FATTR_WUR
sasl_mech_scram_sha2_512_step(struct sasl_session *const restrict p, const struct sasl_input_buf *const restrict in,
                              struct sasl_output_buf *const restrict out)
{
	return scram_step_dispatch(p, in, out, PBKDF2_PRF_SCRAM_SHA2_512_S64);
}

static void
sasl_mech_scram_finish(struct sasl_session *const restrict p)
{
	if (! (p && p->mechdata))
		return;

	struct scram_session *const s = p->mechdata;

	(void) mowgli_node_delete(&s->node, &scram_sessions);
	(void) sfree(s->c_msg_buf);
	(void) smemzerofree(s, sizeof *s);

	p->mechdata = NULL;
}

static const struct sasl_mechanism sasl_mech_scram_sha1 = {
	.name           = "SCRAM-SHA-1",
	.mech_start     = NULL,
	.mech_step      = &sasl_mech_scram_sha1_step,
	.mech_finish    = &sasl_mech_scram_finish,
	.password_based = true,
};

static const struct sasl_mechanism sasl_mech_scram_sha2_256 = {
	.name           = "SCRAM-SHA-256",
	.mech_start     = NULL,
	.mech_step      = &sasl_mech_scram_sha2_256_step,
	.mech_finish    = &sasl_mech_scram_finish,
	.password_based = true,
};

static const struct sasl_mechanism sasl_mech_scram_sha2_512 = {
	.name           = "SCRAM-SHA-512",
	.mech_start     = NULL,
	.mech_step      = &sasl_mech_scram_sha2_512_step,
	.mech_finish    = &sasl_mech_scram_finish,
	.password_based = true,
};

static inline void
scram_mechanisms_register(void)
{
	(void) sasl_core_functions->mech_register(&sasl_mech_scram_sha2_512);
	(void) sasl_core_functions->mech_register(&sasl_mech_scram_sha2_256);
	(void) sasl_core_functions->mech_register(&sasl_mech_scram_sha1);
}

static inline void
scram_mechanisms_unregister(void)
{
	(void) sasl_core_functions->mech_unregister(&sasl_mech_scram_sha2_512);
	(void) sasl_core_functions->mech_unregister(&sasl_mech_scram_sha2_256);
	(void) sasl_core_functions->mech_unregister(&sasl_mech_scram_sha1);
}

static void
scram_pbkdf2v2_scram_confhook(const struct pbkdf2v2_scram_config *const restrict config)
{
	const struct crypt_impl *const ci_default = crypt_get_default_provider();

	if (! ci_default)
		(void) slog(LG_ERROR, "%s: %s is apparently loaded but no crypto provider is available (BUG)",
		                      MOWGLI_FUNC_NAME, PBKDF2V2_CRYPTO_MODULE_NAME);
	else if (strcmp(ci_default->id, PBKDF2V2_CRYPTO_MODULE_NAME) != 0)
		(void) slog(LG_INFO, "%s: %s is not the default crypto provider, PLEASE INVESTIGATE THIS! "
		                     "Newly registered users, and users who change their passwords, will not "
		                     "be able to login with this module until this is rectified.", MOWGLI_FUNC_NAME,
		                     PBKDF2V2_CRYPTO_MODULE_NAME);

	switch (config->a)
	{
		case PBKDF2_PRF_SCRAM_SHA1_S64:
		case PBKDF2_PRF_SCRAM_SHA2_256_S64:
		case PBKDF2_PRF_SCRAM_SHA2_512_S64:
			(void) scram_mechanisms_register();
			break;

		default:
			(void) scram_mechanisms_unregister();
			(void) slog(LG_ERROR, "%s: crypto::pbkdf2v2_digest is not set to a supported value -- "
			                      "this module will not do anything", MOWGLI_FUNC_NAME);
			break;
	}

	if (config->c > CYRUS_SASL_ITERCNT_MAX)
		(void) slog(LG_INFO, "%s: iteration count (%u) is higher than Cyrus SASL library maximum (%u) -- "
		                     "client logins may fail if they use Cyrus", MOWGLI_FUNC_NAME, config->c,
		                     CYRUS_SASL_ITERCNT_MAX);
}

static void
mod_init(struct module *const restrict m)
{
	// Services administrators using this module should be fully aware of the requirements for correctly doing so
	if (! module_find_published(PBKDF2V2_CRYPTO_MODULE_NAME))
	{
		(void) slog(LG_ERROR, "%s: Please read 'doc/SASL-SCRAM'; refusing to load.", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	MODULE_TRY_REQUEST_SYMBOL(m, sasl_core_functions, "saslserv/main", "sasl_core_functions")
	MODULE_TRY_REQUEST_SYMBOL(m, pbkdf2v2_scram_functions, PBKDF2V2_CRYPTO_MODULE_NAME, "pbkdf2v2_scram_functions")

	// Ask the PBKDF2 module to call us back with its configuration
	(void) pbkdf2v2_scram_functions->confhook(&scram_pbkdf2v2_scram_confhook);

	// We need to be told when a user account is deleted in case there is an active SCRAM negotiation for it
	(void) hook_add_myuser_delete(&scram_myuser_delete);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	// Unregister configuration interest in the pbkdf2v2 module
	(void) pbkdf2v2_scram_functions->confhook(NULL);

	// We no longer need this
	(void) hook_del_myuser_delete(&scram_myuser_delete);

	// Unregister the SASL mechanisms
	(void) scram_mechanisms_unregister();
}

#else /* HAVE_LIBIDN */

static void
mod_init(struct module *const restrict m)
{
	(void) slog(LG_ERROR, "Module %s requires GNU libidn support, refusing to load.", m->name);

	m->mflags |= MODFLAG_FAIL;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	// Nothing To Do
}

#endif /* !HAVE_LIBIDN */

SIMPLE_DECLARE_MODULE_V1("saslserv/scram", MODULE_UNLOAD_CAPABILITY_OK)
