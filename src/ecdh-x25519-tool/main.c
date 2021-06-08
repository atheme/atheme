/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Aaron M. D. Jones <me@aaronmdjones.net>
 *
 * Helper utility for the SASL ECDH-X25519-CHALLENGE mechanism.
 */

#include <atheme.h>

#define ECDH_X25519_LOG_TO_STDERR 1

#include "ecdh-x25519-challenge-shared.c"
#include "ecdh-x25519-tool.h"

#include <atheme/libathemecore.h>
#include <ext/getopt_long.h>

void ATHEME_FATTR_PRINTF(1, 2)
print_stderr(const char *const restrict format, ...)
{
	va_list ap;
	va_start(ap, format);
	(void) vfprintf(stderr, format, ap);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	va_end(ap);
}

void ATHEME_FATTR_PRINTF(1, 2)
print_stdout(const char *const restrict format, ...)
{
	va_list ap;
	va_start(ap, format);
	(void) vfprintf(stdout, format, ap);
	(void) fprintf(stdout, "\n");
	(void) fflush(stdout);
	va_end(ap);
}

static int
ecdh_x25519_tool_create_keypair(const char *const restrict keyfile_path)
{
	unsigned char seckey[ATHEME_ECDH_X25519_XKEY_LEN];
	unsigned char pubkey[ATHEME_ECDH_X25519_XKEY_LEN];
	char pubkey_b64[BASE64_SIZE_STR(sizeof pubkey)];

	(void) umask(0066);

	if (keyfile_path != NULL && access(keyfile_path, F_OK) == 0)
	{
		(void) print_stderr(_("Key file '%s' already exists"), keyfile_path);
		return EXIT_FAILURE;
	}

	FILE *const fh = ((keyfile_path != NULL) ? fopen(keyfile_path, "wb") : stdout);

	if (! fh)
	{
		(void) print_stderr("fopen('%s', 'wb'): %s", keyfile_path, strerror(errno));
		return EXIT_FAILURE;
	}

	if (! ecdh_x25519_create_keypair(seckey, pubkey))
		// This function prints error messages on failure
		return EXIT_FAILURE;

	if (fwrite(seckey, sizeof seckey, 1, fh) != 1)
	{
		(void) print_stderr(_("Could not write private key to key file"));
		return EXIT_FAILURE;
	}
	if (fwrite(pubkey, sizeof pubkey, 1, fh) != 1)
	{
		(void) print_stderr(_("Could not write public key to key file"));
		return EXIT_FAILURE;
	}

	(void) fflush(fh);
	(void) fclose(fh);

	if (keyfile_path != NULL)
	{
		if (base64_encode(pubkey, sizeof pubkey, pubkey_b64, sizeof pubkey_b64) == BASE64_FAIL)
		{
			(void) print_stderr(_("base64_encode() failed (BUG?)"));
			return EXIT_FAILURE;
		}

		(void) print_stdout("%s", pubkey_b64);
	}

	return EXIT_SUCCESS;
}

static int
ecdh_x25519_tool_print_pubkey(const char *const restrict keyfile_path)
{
	unsigned char seckey[ATHEME_ECDH_X25519_XKEY_LEN];
	unsigned char pubkey[ATHEME_ECDH_X25519_XKEY_LEN];
	char pubkey_b64[BASE64_SIZE_STR(sizeof pubkey)];

	FILE *const fh = ((keyfile_path != NULL) ? fopen(keyfile_path, "rb") : stdin);

	if (! fh)
	{
		(void) print_stderr("fopen('%s', 'rb'): %s", keyfile_path, strerror(errno));
		return EXIT_FAILURE;
	}
	if (fread(seckey, sizeof seckey, 1, fh) != 1)
	{
		(void) print_stderr(_("Could not read private key from key file"));
		return EXIT_FAILURE;
	}
	if (fread(pubkey, sizeof pubkey, 1, fh) != 1)
	{
		(void) print_stderr(_("Could not read public key from key file"));
		return EXIT_FAILURE;
	}

	(void) fclose(fh);

	if (base64_encode(pubkey, sizeof pubkey, pubkey_b64, sizeof pubkey_b64) == BASE64_FAIL)
	{
		(void) print_stderr(_("base64_encode() failed (BUG?)"));
		return EXIT_FAILURE;
	}

	(void) print_stdout("%s", pubkey_b64);
	return EXIT_SUCCESS;
}

static int
ecdh_x25519_tool_respond(const char *const restrict keyfile_path, const char *const restrict server_message_str)
{
	unsigned char seckey[ATHEME_ECDH_X25519_XKEY_LEN];
	unsigned char pubkey[ATHEME_ECDH_X25519_XKEY_LEN];
	unsigned char shared_secret[ATHEME_ECDH_X25519_XKEY_LEN];
	unsigned char better_secret[ATHEME_ECDH_X25519_CHAL_LEN];
	unsigned char challenge[ATHEME_ECDH_X25519_CHAL_LEN];
	char challenge_b64[BASE64_SIZE_STR(sizeof challenge)];
	union ecdh_x25519_server_response resp;

	FILE *const fh = ((keyfile_path != NULL) ? fopen(keyfile_path, "rb") : stdin);

	if (! fh)
	{
		(void) print_stderr("fopen('%s', 'rb'): %s", keyfile_path, strerror(errno));
		return EXIT_FAILURE;
	}
	if (fread(seckey, sizeof seckey, 1, fh) != 1)
	{
		(void) print_stderr(_("Could not read private key from key file"));
		return EXIT_FAILURE;
	}
	if (fread(pubkey, sizeof pubkey, 1, fh) != 1)
	{
		(void) print_stderr(_("Could not read public key from key file"));
		return EXIT_FAILURE;
	}

	(void) fclose(fh);

	if (base64_decode(server_message_str, resp.octets, sizeof resp.octets) != sizeof resp.octets)
	{
		(void) print_stderr(_("Server message is invalid"));
		return EXIT_FAILURE;
	}

	if (! ecdh_x25519_compute_shared(seckey, resp.field.pubkey, shared_secret))
		// This function prints error messages on failure
		return EXIT_FAILURE;

	if (! ecdh_x25519_kdf(shared_secret, pubkey, resp.field.pubkey, resp.field.salt, better_secret))
		// This function prints error messages on failure
		return EXIT_FAILURE;

	for (size_t x = 0; x < sizeof challenge; x++)
		challenge[x] = resp.field.challenge[x] ^ better_secret[x];

	if (base64_encode(challenge, sizeof challenge, challenge_b64, sizeof challenge_b64) == BASE64_FAIL)
	{
		(void) print_stderr(_("base64_encode() failed (BUG?)"));
		return EXIT_FAILURE;
	}

	(void) print_stdout("%s", challenge_b64);
	return EXIT_SUCCESS;
}

static int
ecdh_x25519_tool_server(const char *const restrict keyfile_path)
{
	unsigned char client_seckey[ATHEME_ECDH_X25519_XKEY_LEN];
	unsigned char client_pubkey[ATHEME_ECDH_X25519_XKEY_LEN];
	unsigned char server_seckey[ATHEME_ECDH_X25519_XKEY_LEN];
	unsigned char server_pubkey[ATHEME_ECDH_X25519_XKEY_LEN];
	unsigned char shared_secret[ATHEME_ECDH_X25519_XKEY_LEN];
	unsigned char better_secret[ATHEME_ECDH_X25519_CHAL_LEN];
	unsigned char challenge[ATHEME_ECDH_X25519_CHAL_LEN];
	union ecdh_x25519_server_response resp;
	char challenge_b64[BASE64_SIZE_STR(sizeof challenge)];
	char resp_octets_b64[BASE64_SIZE_STR(sizeof resp.octets)];

	FILE *const fh = ((keyfile_path != NULL) ? fopen(keyfile_path, "rb") : stdin);

	if (! fh)
	{
		(void) print_stderr("fopen('%s', 'rb'): %s", keyfile_path, strerror(errno));
		return EXIT_FAILURE;
	}
	if (fread(client_seckey, sizeof client_seckey, 1, fh) != 1)
	{
		(void) print_stderr(_("Could not read private key from key file"));
		return EXIT_FAILURE;
	}
	if (fread(client_pubkey, sizeof client_pubkey, 1, fh) != 1)
	{
		(void) print_stderr(_("Could not read public key from key file"));
		return EXIT_FAILURE;
	}

	(void) fclose(fh);

	if (! ecdh_x25519_create_keypair(server_seckey, server_pubkey))
		// This function prints error messages on failure
		return EXIT_FAILURE;

	if (! ecdh_x25519_compute_shared(server_seckey, client_pubkey, shared_secret))
		// This function prints error messages on failure
		return EXIT_FAILURE;

	(void) memcpy(resp.field.pubkey, server_pubkey, sizeof server_pubkey);
	(void) atheme_random_buf(resp.field.salt, sizeof resp.field.salt);
	(void) atheme_random_buf(challenge, sizeof challenge);

	if (! ecdh_x25519_kdf(shared_secret, client_pubkey, server_pubkey, resp.field.salt, better_secret))
		// This function prints error messages on failure
		return EXIT_FAILURE;

	for (size_t x = 0; x < sizeof resp.field.challenge; x++)
		resp.field.challenge[x] = challenge[x] ^ better_secret[x];

	if (base64_encode(challenge, sizeof challenge, challenge_b64, sizeof challenge_b64) == BASE64_FAIL)
	{
		(void) print_stderr(_("base64_encode() failed (BUG?)"));
		return EXIT_FAILURE;
	}
	if (base64_encode(resp.octets, sizeof resp.octets, resp_octets_b64, sizeof resp_octets_b64) == BASE64_FAIL)
	{
		(void) print_stderr(_("base64_encode() failed (BUG?)"));
		return EXIT_FAILURE;
	}

	(void) print_stdout(_("Original challenge: %s"), challenge_b64);
	(void) print_stdout(_("Server message:     %s"), resp_octets_b64);
	return EXIT_SUCCESS;
}

static void
ecdh_x25519_tool_print_help(void)
{
	(void) print_stdout(_(""
		"\n"
		"Usage: ecdh-x25519-tool [-h | -v | -T]\n"
		"       ecdh-x25519-tool [-f <path>] [-c | -p | -q | -r <msg> | -s]\n"
		"\n"
		"  -h / --help               Print this message\n"
		"  -v / --version            Print version information\n"
		"  -T / --selftest-only      Exit after running X25519 ECDH self-test\n"
		"  -f / --key-file <path>    Path to key file for reading or writing\n"
		"  -c / --create-keypair     Write new key pair to file, print public key\n"
		"  -p / --print-pubkey       Read existing key file, print public key\n"
		"  -q / --print-qrcode       Print PRIVATE AND PUBLIC KEY as a QR-Code\n"
		"  -r / --respond <msg>      Generate response to a server message\n"
		"  -s / --server             Generate a server message (for testing)\n"
		"\n"
		"If \"-f\" is not given, \"-f -\" is assumed. This means use stdin/stdout.\n"
		"\n"
		"If \"-c -f -\" is given, the entire RAW key pair will be written to stdout.\n"
		"     Please ensure that stdout is NOT a terminal if this is the case!\n"
	));
}

int
main(int argc, char **argv)
{
	if (! libathemecore_early_init())
		// This function prints error messages on failure
		return EXIT_FAILURE;

	const char short_opts[] = "hvTf:cpqr:s";

	const mowgli_getopt_option_t long_opts[] = {
		{           "help",       no_argument, NULL, 'h', 0 },
		{        "version",       no_argument, NULL, 'v', 0 },
		{  "selftest-only",       no_argument, NULL, 'T', 0 },
		{       "key-file", required_argument, NULL, 'f', 0 },
		{ "create-keypair",       no_argument, NULL, 'c', 0 },
		{   "print-pubkey",       no_argument, NULL, 'p', 0 },
		{   "print-qrcode",       no_argument, NULL, 'q', 0 },
		{        "respond", required_argument, NULL, 'r', 0 },
		{         "server",       no_argument, NULL, 's', 0 },
		{             NULL,                 0, NULL,  0 , 0 },
	};

	const char *server_message_str = NULL;
	const char *keyfile_path = NULL;

	unsigned int options = X25519TOOL_OPTFLAG_NONE;

	int r;

	while ((r = mowgli_getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1)
	{
		switch (r)
		{
			case 'h':
				(void) ecdh_x25519_tool_print_help();
				return EXIT_SUCCESS;

			case 'v':
				(void) print_stdout("This is ecdh-x25519-tool from %s (%s)", PACKAGE_STRING, SERNO);
				return EXIT_SUCCESS;

			case 'T':
				options |= X25519TOOL_OPTFLAG_SELFTEST_ONLY;
				break;

			case 'f':
				keyfile_path = sstrdup(mowgli_optarg);
				break;

			case 'c':
				options |= X25519TOOL_OPTFLAG_CREATE_KEYPAIR;
				break;

			case 'p':
				options |= X25519TOOL_OPTFLAG_PRINT_PUBKEY;
				break;

			case 'q':
				options |= X25519TOOL_OPTFLAG_PRINT_QRCODE;
				break;

			case 'r':
				server_message_str = sstrdup(mowgli_optarg);
				options |= X25519TOOL_OPTFLAG_RESPOND;
				break;

			case 's':
				options |= X25519TOOL_OPTFLAG_SERVER;
				break;

			default:
				(void) ecdh_x25519_tool_print_help();
				return EXIT_FAILURE;
		}
	}

	if (keyfile_path != NULL && strcmp(keyfile_path, "-") == 0)
		keyfile_path = NULL;

	if (! options || (options & (options - 1)))
	{
		(void) ecdh_x25519_tool_print_help();
		return EXIT_FAILURE;
	}

	if (! ecdh_x25519_selftest())
		// This function prints error messages on failure
		return EXIT_FAILURE;

	if ((options & X25519TOOL_OPTFLAG_SELFTEST_ONLY) == X25519TOOL_OPTFLAG_SELFTEST_ONLY)
		return EXIT_SUCCESS;

	if ((options & X25519TOOL_OPTFLAG_CREATE_KEYPAIR) == X25519TOOL_OPTFLAG_CREATE_KEYPAIR)
		// This function prints error messages on failure
		return ecdh_x25519_tool_create_keypair(keyfile_path);

	if ((options & X25519TOOL_OPTFLAG_PRINT_PUBKEY) == X25519TOOL_OPTFLAG_PRINT_PUBKEY)
		// This function prints error messages on failure
		return ecdh_x25519_tool_print_pubkey(keyfile_path);

	if ((options & X25519TOOL_OPTFLAG_PRINT_QRCODE) == X25519TOOL_OPTFLAG_PRINT_QRCODE)
		// This function prints error messages on failure
		return ecdh_x25519_tool_print_qrcode(keyfile_path);

	if ((options & X25519TOOL_OPTFLAG_RESPOND) == X25519TOOL_OPTFLAG_RESPOND)
		// This function prints error messages on failure
		return ecdh_x25519_tool_respond(keyfile_path, server_message_str);

	if ((options & X25519TOOL_OPTFLAG_SERVER) == X25519TOOL_OPTFLAG_SERVER)
		// This function prints error messages on failure
		return ecdh_x25519_tool_server(keyfile_path);

	(void) ecdh_x25519_tool_print_help();
	return EXIT_FAILURE;
}
