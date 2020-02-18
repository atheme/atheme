/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2020 Atheme Development Group (https://atheme.github.io/)
 */

#include <atheme.h>
#include <atheme/pbkdf2.h>

#define SCANFMT_BASE64_CRYPT3       "%[" BASE64_ALPHABET_CRYPT3 "]"
#define SCANFMT_BASE64_RFC4648      "%[" BASE64_ALPHABET_RFC4648 "]"
#define SCANFMT_HEXADECIMAL         "%[A-Fa-f0-9]"

#define SCANFMT_ANOPE_ENC_SHA256    "$anope$enc_sha256$" SCANFMT_BASE64_RFC4648 "$" SCANFMT_BASE64_RFC4648
#define SCANFMT_ARGON2              "$%[A-Za-z0-9]$v=%u$m=%u,t=%u,p=%u$" SCANFMT_BASE64_RFC4648 "$" SCANFMT_BASE64_RFC4648
#define SCANFMT_BASE64              "$base64$" SCANFMT_BASE64_RFC4648
#define SCANFMT_BCRYPT              "$2%1[ab]$%2u$%22[" BASE64_ALPHABET_CRYPT3 "]%31[" BASE64_ALPHABET_CRYPT3 "]"
#define SCANFMT_CRYPT3_DES          SCANFMT_BASE64_CRYPT3
#define SCANFMT_CRYPT3_MD5          "$1$" SCANFMT_BASE64_CRYPT3 "$" SCANFMT_BASE64_CRYPT3
#define SCANFMT_CRYPT3_SHA2_256     "$5$" SCANFMT_BASE64_CRYPT3 "$" SCANFMT_BASE64_CRYPT3
#define SCANFMT_CRYPT3_SHA2_256_EXT "$5$rounds=%u$" SCANFMT_BASE64_CRYPT3 "$" SCANFMT_BASE64_CRYPT3
#define SCANFMT_CRYPT3_SHA2_512     "$6$" SCANFMT_BASE64_CRYPT3 "$" SCANFMT_BASE64_CRYPT3
#define SCANFMT_CRYPT3_SHA2_512_EXT "$6$rounds=%u$" SCANFMT_BASE64_CRYPT3 "$" SCANFMT_BASE64_CRYPT3
#define SCANFMT_IRCSERVICES         "$ircservices$" SCANFMT_HEXADECIMAL
#define SCANFMT_PBKDF2              SCANFMT_HEXADECIMAL
#define SCANFMT_PBKDF2V2_SCRAM      "$z$%u$%u$" SCANFMT_BASE64_RFC4648 "$" SCANFMT_BASE64_RFC4648 "$" SCANFMT_BASE64_RFC4648
#define SCANFMT_PBKDF2V2_HMAC       "$z$%u$%u$" SCANFMT_BASE64_RFC4648 "$" SCANFMT_BASE64_RFC4648
#define SCANFMT_RAWMD5              "$rawmd5$" SCANFMT_HEXADECIMAL
#define SCANFMT_RAWSHA1             "$rawsha1$" SCANFMT_HEXADECIMAL
#define SCANFMT_RAWSHA2_256         "$rawsha256$" SCANFMT_HEXADECIMAL
#define SCANFMT_RAWSHA2_512         "$rawsha512$" SCANFMT_HEXADECIMAL
#define SCANFMT_SCRYPT              "$scrypt$ln=%u,r=%u,p=%u$" SCANFMT_BASE64_CRYPT3 "$" SCANFMT_BASE64_CRYPT3

enum crypto_type
{
	TYPE_NONE = 0,
	TYPE_UNKNOWN,
	TYPE_ANOPE_ENC_SHA256,
	TYPE_ARGON2D,
	TYPE_ARGON2I,
	TYPE_ARGON2ID,
	TYPE_BASE64,
	TYPE_BCRYPT,
	TYPE_CRYPT3_DES,
	TYPE_CRYPT3_MD5,
	TYPE_CRYPT3_SHA2_256,
	TYPE_CRYPT3_SHA2_512,
	TYPE_IRCSERVICES,
	TYPE_PBKDF2,
	TYPE_PBKDF2V2_SCRAM_MD5,
	TYPE_PBKDF2V2_SCRAM_SHA1,
	TYPE_PBKDF2V2_SCRAM_SHA2_256,
	TYPE_PBKDF2V2_SCRAM_SHA2_512,
	TYPE_PBKDF2V2_HMAC_MD5,
	TYPE_PBKDF2V2_HMAC_SHA1,
	TYPE_PBKDF2V2_HMAC_SHA2_256,
	TYPE_PBKDF2V2_HMAC_SHA2_512,
	TYPE_RAWMD5,
	TYPE_RAWSHA1,
	TYPE_RAWSHA2_256,
	TYPE_RAWSHA2_512,
	TYPE_SCRYPT,
	TYPE_TOTAL_COUNT,
};

static const char *
crypto_type_to_name(const enum crypto_type type)
{
	switch (type)
	{
		case TYPE_NONE:
			return "NONE (\00304PLAIN-TEXT!\003)";
		case TYPE_UNKNOWN:
			return "(unknown)";
		case TYPE_ANOPE_ENC_SHA256:
			return "crypto/anope-enc-sha256 (SHA2-256)";
		case TYPE_ARGON2D:
			return "crypto/argon2 (Argon2d)";
		case TYPE_ARGON2I:
			return "crypto/argon2 (Argon2i)";
		case TYPE_ARGON2ID:
			return "crypto/argon2 (Argon2id)";
		case TYPE_BASE64:
			return "crypto/base64 (\00304PLAIN-TEXT!\003)";
		case TYPE_BCRYPT:
			return "crypto/bcrypt (EksBlowfish)";
		case TYPE_CRYPT3_DES:
			return "crypto/crypt3-des (DES)";
		case TYPE_CRYPT3_MD5:
			return "crypto/crypt3-md5 (MD5)";
		case TYPE_CRYPT3_SHA2_256:
			return "crypto/crypt3-sha2-256 (SHA2-256)";
		case TYPE_CRYPT3_SHA2_512:
			return "crypto/crypt3-sha2-512 (SHA2-512)";
		case TYPE_IRCSERVICES:
			return "crypto/ircservices (MD5)";
		case TYPE_PBKDF2:
			return "crypto/pbkdf2 (HMAC-SHA2-512)";
		case TYPE_PBKDF2V2_HMAC_MD5:
			return "crypto/pbkdf2v2 (HMAC-MD5)";
		case TYPE_PBKDF2V2_HMAC_SHA1:
			return "crypto/pbkdf2v2 (HMAC-SHA1)";
		case TYPE_PBKDF2V2_HMAC_SHA2_256:
			return "crypto/pbkdf2v2 (HMAC-SHA2-256)";
		case TYPE_PBKDF2V2_HMAC_SHA2_512:
			return "crypto/pbkdf2v2 (HMAC-SHA2-512)";
		case TYPE_PBKDF2V2_SCRAM_MD5:
			return "crypto/pbkdf2v2 (SCRAM-MD5)";
		case TYPE_PBKDF2V2_SCRAM_SHA1:
			return "crypto/pbkdf2v2 (SCRAM-SHA-1)";
		case TYPE_PBKDF2V2_SCRAM_SHA2_256:
			return "crypto/pbkdf2v2 (SCRAM-SHA-256)";
		case TYPE_PBKDF2V2_SCRAM_SHA2_512:
			return "crypto/pbkdf2v2 (SCRAM-SHA-512)";
		case TYPE_RAWMD5:
			return "crypto/rawmd5 (MD5)";
		case TYPE_RAWSHA1:
			return "crypto/rawsha1 (SHA1)";
		case TYPE_RAWSHA2_256:
			return "crypto/rawsha2-256 (SHA2-256)";
		case TYPE_RAWSHA2_512:
			return "crypto/rawsha2-512 (SHA2-512)";
		case TYPE_SCRYPT:
			return "crypto/sodium-scrypt (scrypt)";
		case TYPE_TOTAL_COUNT:
			// Just to silence diagnostics
			break;
	}

	return NULL;
}

static void
ss_cmd_pwhashes_func(struct sourceinfo *const restrict si, const int ATHEME_VATTR_UNUSED parc,
                     char ATHEME_VATTR_UNUSED **const restrict parv)
{
	(void) logcommand(si, CMDLOG_GET, "PWHASHES");

	unsigned int pwhashes[TYPE_TOTAL_COUNT];

	(void) memset(&pwhashes, 0x00, sizeof pwhashes);

	struct myentity *mt;
	struct myentity_iteration_state state;

	char s1[BUFSIZE];
	char s2[BUFSIZE];
	char s3[BUFSIZE];
	char s4[BUFSIZE];
	unsigned int i1;
	unsigned int i2;
	unsigned int i3;
	unsigned int i4;

	MYENTITY_FOREACH_T(mt, &state, ENT_USER)
	{
		const struct myuser *const mu = user(mt);

		continue_if_fail(mu != NULL);

		const char *const pw = mu->pass;
		const size_t pwlen = strlen(pw);

		if (! (mu->flags & MU_CRYPTPASS))
		{
			pwhashes[TYPE_NONE]++;
		}
		else if (sscanf(pw, SCANFMT_ANOPE_ENC_SHA256, s1, s2) == 2)
		{
			pwhashes[TYPE_ANOPE_ENC_SHA256]++;
		}
		else if (sscanf(pw, SCANFMT_ARGON2, s1, &i1, &i2, &i3, &i4, s2, s3) == 7)
		{
			if (strcasecmp(s1, "argon2d") == 0)
				pwhashes[TYPE_ARGON2D]++;
			else if (strcasecmp(s1, "argon2i") == 0)
				pwhashes[TYPE_ARGON2I]++;
			else if (strcasecmp(s1, "argon2id") == 0)
				pwhashes[TYPE_ARGON2ID]++;
			else
				pwhashes[TYPE_UNKNOWN]++;
		}
		else if (sscanf(pw, SCANFMT_BASE64, s1) == 1)
		{
			pwhashes[TYPE_BASE64]++;
		}
		else if (pwlen >= 60U && sscanf(pw, SCANFMT_BCRYPT, s1, &i1, s2, s3) == 4)
		{
			pwhashes[TYPE_BCRYPT]++;
		}
		else if (pwlen == 13U && sscanf(pw, SCANFMT_CRYPT3_DES, s1) == 1 && strcmp(s1, pw) == 0)
		{
			// Fuzzy (no rigid format)
			pwhashes[TYPE_CRYPT3_DES]++;
		}
		else if (sscanf(pw, SCANFMT_CRYPT3_MD5, s1, s2) == 2)
		{
			pwhashes[TYPE_CRYPT3_MD5]++;
		}
		else if (sscanf(pw, SCANFMT_CRYPT3_SHA2_256, s1, s2) == 2)
		{
			pwhashes[TYPE_CRYPT3_SHA2_256]++;
		}
		else if (sscanf(pw, SCANFMT_CRYPT3_SHA2_256_EXT, &i1, s1, s2) == 3)
		{
			pwhashes[TYPE_CRYPT3_SHA2_256]++;
		}
		else if (sscanf(pw, SCANFMT_CRYPT3_SHA2_512, s1, s2) == 2)
		{
			pwhashes[TYPE_CRYPT3_SHA2_512]++;
		}
		else if (sscanf(pw, SCANFMT_CRYPT3_SHA2_512_EXT, &i1, s1, s2) == 3)
		{
			pwhashes[TYPE_CRYPT3_SHA2_512]++;
		}
		else if (sscanf(pw, SCANFMT_IRCSERVICES, s1) == 1)
		{
			pwhashes[TYPE_IRCSERVICES]++;
		}
		else if (pwlen == 140U && sscanf(pw, SCANFMT_PBKDF2, s1) == 1 && strcmp(s1, pw) == 0)
		{
			// Fuzzy (no rigid format)
			pwhashes[TYPE_PBKDF2]++;
		}
		else if (sscanf(pw, SCANFMT_PBKDF2V2_SCRAM, &i1, &i2, s1, s2, s3) == 5)
		{
			switch (i1)
			{
				case PBKDF2_PRF_SCRAM_MD5:
				case PBKDF2_PRF_SCRAM_MD5_S64:
					pwhashes[TYPE_PBKDF2V2_SCRAM_MD5]++;
					break;
				case PBKDF2_PRF_SCRAM_SHA1:
				case PBKDF2_PRF_SCRAM_SHA1_S64:
					pwhashes[TYPE_PBKDF2V2_SCRAM_SHA1]++;
					break;
				case PBKDF2_PRF_SCRAM_SHA2_256:
				case PBKDF2_PRF_SCRAM_SHA2_256_S64:
					pwhashes[TYPE_PBKDF2V2_SCRAM_SHA2_256]++;
					break;
				case PBKDF2_PRF_SCRAM_SHA2_512:
				case PBKDF2_PRF_SCRAM_SHA2_512_S64:
					pwhashes[TYPE_PBKDF2V2_SCRAM_SHA2_512]++;
					break;
				default:
					pwhashes[TYPE_UNKNOWN]++;
					break;
			}
		}
		else if (sscanf(pw, SCANFMT_PBKDF2V2_HMAC, &i1, &i2, s1, s2) == 4)
		{
			switch (i1)
			{
				case PBKDF2_PRF_HMAC_MD5:
				case PBKDF2_PRF_HMAC_MD5_S64:
					pwhashes[TYPE_PBKDF2V2_HMAC_MD5]++;
					break;
				case PBKDF2_PRF_HMAC_SHA1:
				case PBKDF2_PRF_HMAC_SHA1_S64:
					pwhashes[TYPE_PBKDF2V2_HMAC_SHA1]++;
					break;
				case PBKDF2_PRF_HMAC_SHA2_256:
				case PBKDF2_PRF_HMAC_SHA2_256_S64:
					pwhashes[TYPE_PBKDF2V2_HMAC_SHA2_256]++;
					break;
				case PBKDF2_PRF_HMAC_SHA2_512:
				case PBKDF2_PRF_HMAC_SHA2_512_S64:
					pwhashes[TYPE_PBKDF2V2_HMAC_SHA2_512]++;
					break;
				default:
					pwhashes[TYPE_UNKNOWN]++;
					break;
			}
		}
		else if (sscanf(pw, SCANFMT_RAWMD5, s1) == 1)
		{
			pwhashes[TYPE_RAWMD5]++;
		}
		else if (sscanf(pw, SCANFMT_RAWSHA1, s1) == 1)
		{
			pwhashes[TYPE_RAWSHA1]++;
		}
		else if (sscanf(pw, SCANFMT_RAWSHA2_256, s1) == 1)
		{
			pwhashes[TYPE_RAWSHA2_256]++;
		}
		else if (sscanf(pw, SCANFMT_RAWSHA2_512, s1) == 1)
		{
			pwhashes[TYPE_RAWSHA2_512]++;
		}
		else if (sscanf(pw, SCANFMT_SCRYPT, &i1, &i2, &i3, s1, s2) == 5)
		{
			pwhashes[TYPE_SCRYPT]++;
		}
		else
		{
			pwhashes[TYPE_UNKNOWN]++;
		}
	}

	(void) smemzero(s1, sizeof s1);
	(void) smemzero(s2, sizeof s2);
	(void) smemzero(s3, sizeof s3);
	(void) smemzero(s4, sizeof s4);

	for (enum crypto_type i = TYPE_NONE; i < TYPE_TOTAL_COUNT; i++)
		if (pwhashes[i])
			(void) command_success_nodata(si, "%-36s: %u", crypto_type_to_name(i), pwhashes[i]);
}

static struct command ss_cmd_pwhashes = {
	.name           = "PWHASHES",
	.desc           = N_("Shows database password hash statistics."),
	.access         = PRIV_ADMIN,
	.maxparc        = 1,
	.cmd            = &ss_cmd_pwhashes_func,
	.help           = { .path = "statserv/pwhashes" },
};

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "statserv/main")

	(void) service_named_bind_command("statserv", &ss_cmd_pwhashes);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) service_named_unbind_command("statserv", &ss_cmd_pwhashes);
}

SIMPLE_DECLARE_MODULE_V1("statserv/pwhashes", MODULE_UNLOAD_CAPABILITY_OK)
