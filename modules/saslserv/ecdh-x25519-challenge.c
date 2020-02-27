/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * ECDH-X25519-CHALLENGE mechanism provider.
 *
 * This is a client-first SASL mechanism.
 *
 * The first message from the client to the server consists of an authcid (or,
 * optionally, an authcid, followed by a null octet, followed by an authzid).
 * The latter allows SASL impersonation. Clients MAY set both the authcid and
 * the authzid to the same value.
 *
 *     client_message = authcid [ 0x00 authzid ]
 *
 * The server then does the following:
 *
 *     client_pubkey = fetch_configured_pubkey(authcid)
 * ++  server_seckey = Curve25519_clamp(CSPRNG(32))
 * ++  server_pubkey = Curve25519(server_seckey, basepoint)
 *     shared_secret = Curve25519(server_seckey, client_pubkey)
 *     session_salt = CSPRNG(32)
 *     better_secret = ECDH-X25519-KDF()
 *     session_challenge = CSPRNG(32)
 *     masked_challenge = (session_challenge XOR better_secret)
 *     server_response = server_pubkey || session_salt || masked_challenge
 *
 * The client then does the following:
 *
 *     client_seckey = fetch_configured_seckey()
 *     client_pubkey = Curve25519(client_seckey, basepoint)
 *     shared_secret = Curve25519(client_seckey, server_pubkey)
 *     better_secret = ECDH-X25519-KDF()
 *     session_challenge = (masked_challenge XOR better_secret)
 *     client_response = session_challenge
 *
 * If the client possesses the correct private key, the secret derived on both
 * sides will be identical, and thus the client response will be the correct
 * session challenge, which is verified by the server against the session
 * challenge it generated.
 *
 * If the challenges match, the client is logged in as the authzid, or, if an
 * authzid was not provided, the authcid.
 *
 * For the definitions of Curve25519_clamp(), Curve25519(), and basepoint,
 * please see [1][2].
 *
 * This mechanism is intended to eventually replace ECDSA-NIST256P-CHALLENGE,
 * due to concerns within the cryptographic community about the continued
 * safety and reliability of the NIST curves, and P-256 in particular [3][4].
 *
 * Curve25519 ECDH was chosen over Ed25519 EdDSA as the basis for this
 * mechanism for three primary reasons:
 *
 *   * Curve25519 ECDH was carefully-designed to allow all possible values as
 *     a valid public key, removing the need for the server to validate the
 *     keys, or to worry about operating with potentially-malicious key data.
 *
 *   * Verification of the client's response is a simple constant-time memory
 *     comparison operation, avoiding exposing the server's cryptographic guts
 *     to potentially-malicious client data.
 *
 *   * More crypto libraries implement Curve25519 ECDH than Ed25519 EdDSA as
 *     of this writing.
 *
 * Additionally, key agreement is faster than signature verification, but that
 * does not turn out to be of any consequence here because of the HKDF below:
 *
 * The ECDH-X25519-KDF() function is defined as follows:
 *
 *     ECDH-X25519-KDF() = {
 *         IKM = Hash(shared_secret || client_pubkey || server_pubkey)
 *         PRK = HKDF-Extract(session_salt, IKM)
 *         OKM = HKDF-Expand(PRK, "ECDH-X25519-CHALLENGE", L=32)
 *               // The string above is 21 ASCII octets
 *         return OKM
 *     }
 *
 * The digest algorithm used for Hash(), HKDF-Extract(), and HKDF-Expand(), is
 * SHA2-256. The HKDF-Extract() and HKDF-Expand() functions, and the order and
 * meaning of their arguments, are as described in Section 2 of RFC5869 [5].
 *
 * Note that both the client's and server's Curve25519 public keys are used in
 * the key derivation process in, and the Curve25519 shared secret is derived
 * in, and the server's public key is transmitted to the client in, the
 * canonical Curve25519 key representation format of a string of 32 octets,
 * representing a **little-endian** 256-bit integer. This diverges from the
 * canonical network octet-order (big-endian), but it is specified as such in
 * Section 5 of RFC7748 [6]. If your cryptographic library reads and writes
 * secret keys, public keys, and shared secrets, in big-endian format, you
 * will need to reverse the string's octets; swap the 1st and 32nd octets,
 * swap the 2nd and 31st octets, swap the 3rd and 30th octets, ... etc. As of
 * this writing, the only known cryptographic library with this octet-order
 * requirement that supports Curve25519 ECDH key agreement is ARM mbedTLS [7].
 *
 * The first half of the shared secret derivation process is justified in [8].
 * The second half of the shared secret derivation process is to ensure that
 * the derived shared secret is cryptographically bound to this application
 * (by using the mechanism name as an input), should the client end up reusing
 * their private key for something else, and to ensure that it is safe for the
 * server to reuse its private key across sessions (by using the session salt
 * as an input -- this also ensures contributory behaviour on the part of the
 * server), should it wish to do so for efficiency:
 *
 * ++ These steps are not necessary for each session; the server MAY perform
 *    them at a regular interval instead, if it wishes to forgo the expense of
 *    deriving a new key-pair for each and every session. Servers SHOULD NOT
 *    perform this only once (on startup, or as statically-configured); a
 *    RECOMMENDED keypair regeneration interval is once per hour.
 *
 *
 *
 * [1] <https://cr.yp.to/ecdh.html>
 * [2] <https://cr.yp.to/ecdh/curve25519-20060209.pdf>
 * [3] <https://safecurves.cr.yp.to/>
 * [4] <https://cr.yp.to/talks/2013.05.31/slides-dan+tanja-20130531-4x3.pdf>
 * [5] <https://tools.ietf.org/html/rfc5869#section-2>
 * [6] <https://tools.ietf.org/html/rfc7748#section-5>
 * [7] <https://github.com/ARMmbed/mbedTLS/issues/2490>
 * [8] <https://download.libsodium.org/doc/advanced/scalar_multiplication>
 *
 *     The number of possible keys is limited to the group size (~~ 2^252),
 *     which is smaller than the key space. For this reason, and to mitigate
 *     subtle attacks due to the fact many (p, n) pairs produce the same
 *     result, using the output of the multiplication q directly as a shared
 *     key is not recommended. A better way to compute a shared key is
 *     H(q || pk1 || pk2), with pk1 and pk2 being the public keys.
 */

#include <atheme.h>

#ifdef HAVE_ANYLIB_ECDH_X25519

#include "ecdh-x25519-challenge-shared.c"

#define ATHEME_ECDH_X25519_KEY_REGEN_INTERVAL   SECONDS_PER_HOUR
#define ATHEME_ECDH_X25519_PUBKEY_MDNAME        "private:x25519pubkey"

static mowgli_patricia_t **ns_set_cmdtree = NULL;
static const struct sasl_core_functions *sasl_core_functions = NULL;

static mowgli_eventloop_timer_t *ecdh_x25519_keypair_regen_timer = NULL;
static unsigned char ecdh_x25519_server_seckey[ATHEME_ECDH_X25519_XKEY_LEN];
static unsigned char ecdh_x25519_server_pubkey[ATHEME_ECDH_X25519_XKEY_LEN];

static bool
ecdh_x25519_keypair_regen(void)
{
	unsigned char seckey[ATHEME_ECDH_X25519_XKEY_LEN];
	unsigned char pubkey[ATHEME_ECDH_X25519_XKEY_LEN];

	if (! ecdh_x25519_create_keypair(seckey, pubkey))
		// This function logs messages on failure
		return false;

	(void) memcpy(ecdh_x25519_server_seckey, seckey, sizeof seckey);
	(void) memcpy(ecdh_x25519_server_pubkey, pubkey, sizeof pubkey);
	(void) smemzero(seckey, sizeof seckey);
	return true;
}

static void
ecdh_x25519_keypair_regen_cb(void ATHEME_VATTR_UNUSED *const restrict vptr)
{
	// This function logs messages on failure
	(void) ecdh_x25519_keypair_regen();
}

static enum sasl_mechanism_result ATHEME_FATTR_WUR
ecdh_x25519_sasl_step_account_names(struct sasl_session *const restrict p,
                                    const struct sasl_input_buf *const restrict in,
                                    struct sasl_output_buf *const restrict out)
{
	unsigned char client_pubkey[ATHEME_ECDH_X25519_XKEY_LEN];
	unsigned char shared_secret[ATHEME_ECDH_X25519_XKEY_LEN];
	unsigned char better_secret[ATHEME_ECDH_X25519_CHAL_LEN];
	unsigned char challenge[ATHEME_ECDH_X25519_CHAL_LEN];
	static union ecdh_x25519_server_response resp;

	enum sasl_mechanism_result retval = ASASL_MRESULT_ERROR;
	struct metadata *md = NULL;
	struct myuser *mu = NULL;

	const char *const end = memchr(in->buf, 0x00, in->len);

	if (! end)
	{
		if (in->len > NICKLEN)
		{
			(void) slog(LG_DEBUG, "%s: in->len (%zu) is unacceptable", MOWGLI_FUNC_NAME, in->len);
			goto cleanup;
		}
		if (! sasl_core_functions->authcid_can_login(p, in->buf, &mu))
		{
			(void) slog(LG_DEBUG, "%s: authcid_can_login failed", MOWGLI_FUNC_NAME);
			goto cleanup;
		}
	}
	else
	{
		const char *const bufchrptr = in->buf;
		const size_t authcid_length = (size_t) (end - bufchrptr);
		const size_t authzid_length = in->len - 1 - authcid_length;

		if (! authcid_length || authcid_length > NICKLEN)
		{
			(void) slog(LG_DEBUG, "%s: authcid_length (%zu) is unacceptable",
			                      MOWGLI_FUNC_NAME, authcid_length);
			goto cleanup;
		}
		if (! authzid_length || authzid_length > NICKLEN)
		{
			(void) slog(LG_DEBUG, "%s: authzid_length (%zu) is unacceptable",
			                      MOWGLI_FUNC_NAME, authzid_length);
			goto cleanup;
		}
		if (! sasl_core_functions->authzid_can_login(p, end + 1, NULL))
		{
			(void) slog(LG_DEBUG, "%s: authzid_can_login failed", MOWGLI_FUNC_NAME);
			goto cleanup;
		}
		if (! sasl_core_functions->authcid_can_login(p, in->buf, &mu))
		{
			(void) slog(LG_DEBUG, "%s: authcid_can_login failed", MOWGLI_FUNC_NAME);
			goto cleanup;
		}
	}
	if (! mu)
	{
		(void) slog(LG_ERROR, "%s: authcid_can_login did not set 'mu' (BUG)", MOWGLI_FUNC_NAME);
		goto cleanup;
	}
	if (! (md = metadata_find(mu, ATHEME_ECDH_X25519_PUBKEY_MDNAME)))
	{
		(void) slog(LG_DEBUG, "%s: metadata_find() failed", MOWGLI_FUNC_NAME);
		goto cleanup;
	}
	if (base64_decode(md->value, client_pubkey, sizeof client_pubkey) != sizeof client_pubkey)
	{
		(void) slog(LG_ERROR, "%s: base64_decode() failed (BUG!)", MOWGLI_FUNC_NAME);
		goto cleanup;
	}

	if (! ecdh_x25519_compute_shared(ecdh_x25519_server_seckey, client_pubkey, shared_secret))
		// This function logs messages on failure
		goto cleanup;

	(void) memcpy(resp.field.pubkey, ecdh_x25519_server_pubkey, sizeof resp.field.pubkey);
	(void) atheme_random_buf(resp.field.salt, sizeof resp.field.salt);

	if (! ecdh_x25519_kdf(shared_secret, client_pubkey, resp.field.pubkey, resp.field.salt, better_secret))
		// This function logs messages on failure
		goto cleanup;

	(void) atheme_random_buf(challenge, sizeof challenge);

	p->mechdata = smemdup(challenge, sizeof challenge);

	for (size_t x = 0; x < sizeof resp.field.challenge; x++)
		resp.field.challenge[x] = challenge[x] ^ better_secret[x];

	out->buf = resp.octets;
	out->len = sizeof resp.octets;

	retval = ASASL_MRESULT_CONTINUE;

cleanup:
	(void) smemzero(shared_secret, sizeof shared_secret);
	(void) smemzero(better_secret, sizeof better_secret);
	(void) smemzero(challenge, sizeof challenge);
	return retval;
}

static enum sasl_mechanism_result ATHEME_FATTR_WUR
ecdh_x25519_sasl_step_verify_challenge(struct sasl_session *const restrict p,
                                       const struct sasl_input_buf *const restrict in)
{
	if (in->len != ATHEME_ECDH_X25519_CHAL_LEN)
	{
		(void) slog(LG_DEBUG, "%s: in->len (%zu) is unacceptable", MOWGLI_FUNC_NAME, in->len);
		return ASASL_MRESULT_ERROR;
	}
	if (smemcmp(p->mechdata, in->buf, in->len) != 0)
	{
		(void) slog(LG_DEBUG, "%s: challenge is incorrect", MOWGLI_FUNC_NAME);
		return ASASL_MRESULT_FAILURE;
	}

	(void) slog(LG_DEBUG, "%s: authentication successful", MOWGLI_FUNC_NAME);
	return ASASL_MRESULT_SUCCESS;
}

static enum sasl_mechanism_result ATHEME_FATTR_WUR
ecdh_x25519_sasl_step(struct sasl_session *const restrict p,
                      const struct sasl_input_buf *const restrict in,
                      struct sasl_output_buf *const restrict out)
{
	if (! (in && in->buf && in->len))
	{
		(void) slog(LG_DEBUG, "%s: client did not send any data", MOWGLI_FUNC_NAME);
		return ASASL_MRESULT_ERROR;
	}

	if (p->mechdata == NULL)
		return ecdh_x25519_sasl_step_account_names(p, in, out);
	else
		return ecdh_x25519_sasl_step_verify_challenge(p, in);
}

static void
ecdh_x25519_sasl_finish(struct sasl_session *const restrict p)
{
	if (! (p && p->mechdata))
		return;

	(void) sfree(p->mechdata);

	p->mechdata = NULL;
}

static void
ns_cmd_set_x25519_pubkey_func(struct sourceinfo *const restrict si, const int parc, char **const restrict parv)
{
	struct metadata *const md = metadata_find(si->smu, ATHEME_ECDH_X25519_PUBKEY_MDNAME);

	if (! parc)
	{
		if (! md)
		{
			(void) command_fail(si, fault_nochange, _("Your public key was not set."));
			return;
		}

		(void) metadata_delete(si->smu, ATHEME_ECDH_X25519_PUBKEY_MDNAME);
		(void) logcommand(si, CMDLOG_SET, "SET:X25519-PUBKEY:REMOVE");
		(void) command_success_nodata(si, _("Your public key entry has been deleted."));
		return;
	}

	unsigned char pubkey[ATHEME_ECDH_X25519_XKEY_LEN];
	char pubkey_b64[BASE64_SIZE_STR(sizeof pubkey)];

	if (base64_decode(parv[0], pubkey, sizeof pubkey) != sizeof pubkey)
	{
		(void) command_fail(si, fault_badparams, _("The public key specified is not valid."));
		return;
	}
	if (base64_encode(pubkey, sizeof pubkey, pubkey_b64, sizeof pubkey_b64) != BASE64_SIZE_RAW(sizeof pubkey))
	{
		(void) command_fail(si, fault_badparams, _("The public key specified is not valid."));
		return;
	}
	if (md && strcmp(md->value, pubkey_b64) == 0)
	{
		(void) command_fail(si, fault_nochange, _("Your public key is already set to \2%s\2."), pubkey_b64);
		return;
	}

	(void) metadata_add(si->smu, ATHEME_ECDH_X25519_PUBKEY_MDNAME, pubkey_b64);
	(void) logcommand(si, CMDLOG_SET, "SET:X25519-PUBKEY: \2%s\2", pubkey_b64);
	(void) command_success_nodata(si, _("Your public key is now set to \2%s\2."), pubkey_b64);
}

static const struct sasl_mechanism sasl_mech_ecdh_x25519_challenge = {
	.name           = "ECDH-X25519-CHALLENGE",
	.mech_start     = NULL,
	.mech_step      = &ecdh_x25519_sasl_step,
	.mech_finish    = &ecdh_x25519_sasl_finish,
	.password_based = false,
};

static struct command ns_cmd_set_x25519_pubkey = {
	.name           = "X25519-PUBKEY",
	.desc           = N_("Changes your ECDH-X25519-CHALLENGE public key."),
	.access         = AC_AUTHENTICATED,
	.maxparc        = 1,
	.cmd            = &ns_cmd_set_x25519_pubkey_func,
	.help           = { .path = "nickserv/set_x25519_pubkey" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, ns_set_cmdtree, "nickserv/set_core", "ns_set_cmdtree")
	MODULE_TRY_REQUEST_SYMBOL(m, sasl_core_functions, "saslserv/main", "sasl_core_functions")

	if (! ecdh_x25519_selftest())
	{
		(void) slog(LG_ERROR, "%s: self-test failed (BUG?); refusing to load", m->name);
		m->mflags |= MODFLAG_FAIL;
		return;
	}
	if (! ecdh_x25519_keypair_regen())
	{
		(void) slog(LG_ERROR, "%s: keypair generation failed (BUG?); refusing to load", m->name);
		m->mflags |= MODFLAG_FAIL;
		return;
	}
	if (! (ecdh_x25519_keypair_regen_timer = mowgli_timer_add(base_eventloop, "ecdh_x25519_keypair_regen",
	       &ecdh_x25519_keypair_regen_cb, NULL, ATHEME_ECDH_X25519_KEY_REGEN_INTERVAL)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_timer_add() failed (BUG?); refusing to load", m->name);
		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) sasl_core_functions->mech_register(&sasl_mech_ecdh_x25519_challenge);
	(void) command_add(&ns_cmd_set_x25519_pubkey, *ns_set_cmdtree);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) smemzero(ecdh_x25519_server_seckey, sizeof ecdh_x25519_server_seckey);
	(void) mowgli_timer_destroy(base_eventloop, ecdh_x25519_keypair_regen_timer);
	(void) sasl_core_functions->mech_unregister(&sasl_mech_ecdh_x25519_challenge);
	(void) command_delete(&ns_cmd_set_x25519_pubkey, *ns_set_cmdtree);
}

#else /* HAVE_ANYLIB_ECDH_X25519 */

static void
mod_init(struct module *const restrict m)
{
	(void) slog(LG_ERROR, "Module %s requires a library with usable Curve25519 key generation "
	                      "& ECDH key agreement functions, refusing to load.", m->name);

	m->mflags |= MODFLAG_FAIL;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	// Nothing To Do
}

#endif /* !HAVE_ANYLIB_ECDH_X25519 */

SIMPLE_DECLARE_MODULE_V1("saslserv/ecdh-x25519-challenge", MODULE_UNLOAD_CAPABILITY_OK)
