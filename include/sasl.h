/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2006 Atheme Project (http://atheme.org/)
 *
 * Data structures and macros for SASL mechanisms.
 */

#include "stdheaders.h"

#ifndef ATHEME_INC_SASL_H
#define ATHEME_INC_SASL_H 1

#include "attrs.h"
#include "common.h"
#include "sourceinfo.h"
#include "structures.h"

// Maximum number of parameters for an SASL S2S command (arbitrary, increment in future if necessary)
#define SASL_MESSAGE_MAXPARA        8

// Maximum length of an SASL mechanism name (including terminating NULL byte)
#define SASL_MECHANISM_MAXLEN       60

// Maximum length of data that can be transferred in one shot
#define SASL_S2S_MAXLEN_ATONCE_RAW  300U
#define SASL_S2S_MAXLEN_ATONCE_B64  400U

// Maximum length of data that can be buffered as one message/request
#define SASL_S2S_MAXLEN_TOTAL_RAW   6144U
#define SASL_S2S_MAXLEN_TOTAL_B64   8192U

// Flags for sasl_session->flags
#define ASASL_MARKED_FOR_DELETION   0x00000001U // see delete_stale() in saslserv/main.c
#define ASASL_NEED_LOG              0x00000002U // user auth success needs to be logged still

// Flags for sasl_input_buf->flags
#define ASASL_INFLAG_NONE           0x00000000U // Nothing special
#define ASASL_INFLAG_WIPE_BUF       0x00000001U // Call smemzero() on the input buffers after processing them

// Flags for sasl_output_buf->flags
#define ASASL_OUTFLAG_NONE          0x00000000U // Nothing special
#define ASASL_OUTFLAG_DONT_FREE_BUF 0x00000001U // Don't sfree() the output buffer after base64-encoding it
#define ASASL_OUTFLAG_WIPE_BUF      0x00000002U // Call smemzero() on the output buffers after processing them

// Return values for sasl_mechanism -> mech_start() or mech_step()
#define ASASL_FAIL                  0U  // client supplied invalid credentials / screwed up their formatting
#define ASASL_MORE                  1U  // everything looks good so far, but we're not done yet
#define ASASL_DONE                  2U  // client successfully authenticated
#define ASASL_ERROR                 3U  // an error occurred in mech or it doesn't want to bad_password() the user

struct sasl_session
{
	mowgli_node_t                   node;                   // Node for entry into the active sessions list
	const struct sasl_mechanism *   mechptr;                // Mechanism they're using
	struct server *                 server;                 // Server they're on
	struct sourceinfo *             si;                     // The source info for logcommand(), bad_password(), and login hooks
	char *                          uid;                    // Network UID
	char *                          buf;                    // Buffered Base-64 data from them (so far)
	void *                          mechdata;               // Mechanism-specific allocated memory
	char *                          certfp;                 // TLS client certificate fingerprint (if any)
	char *                          host;                   // Hostname
	char *                          ip;                     // IP address
	char                            authcid[NICKLEN + 1];   // Authentication identity (user having credentials verified)
	char                            authzid[NICKLEN + 1];   // Authorization identity (user being logged in)
	char                            authceid[IDLEN + 1];    // Entity ID for authcid
	char                            authzeid[IDLEN + 1];    // Entity ID for authzid
	size_t                          len;                    // Length of buffered Base-64 data
	unsigned int                    flags;                  // Flags (described above)
	bool                            tls;                    // Whether their connection to the network is using TLS
};

struct sasl_sourceinfo
{
	struct sourceinfo       parent;
	struct sasl_session *   sess;
};

struct sasl_message
{
	struct server * server;
	char *          uid;
	char *          parv[SASL_MESSAGE_MAXPARA];
	int             parc;
	char            mode;
};

struct sasl_input_buf
{
	const void *    buf;
	const size_t    len;
	unsigned int *  flags;
};

struct sasl_output_buf
{
	void *          buf;
	size_t          len;
	unsigned int    flags;
};

typedef unsigned int (*sasl_mech_start_fn)(struct sasl_session *restrict, struct sasl_output_buf *restrict)
    ATHEME_FATTR_WUR;

typedef unsigned int (*sasl_mech_step_fn)(struct sasl_session *restrict, const struct sasl_input_buf *restrict,
                                          struct sasl_output_buf *restrict) ATHEME_FATTR_WUR;

typedef void (*sasl_mech_finish_fn)(struct sasl_session *);

struct sasl_mechanism
{
	char                name[SASL_MECHANISM_MAXLEN];
	sasl_mech_start_fn  mech_start;
	sasl_mech_step_fn   mech_step;
	sasl_mech_finish_fn mech_finish;
};

typedef bool (*sasl_authxid_can_login_fn)(struct sasl_session *restrict, const char *restrict,
                                          struct myuser **restrict) ATHEME_FATTR_WUR;

struct sasl_core_functions
{
	void                      (*mech_register)(const struct sasl_mechanism *);
	void                      (*mech_unregister)(const struct sasl_mechanism *);
	sasl_authxid_can_login_fn   authcid_can_login;
	sasl_authxid_can_login_fn   authzid_can_login;
};

typedef struct {

	struct myuser * source_mu;
	struct myuser * target_mu;
	bool            allowed;

} hook_sasl_may_impersonate_t;

#endif /* !ATHEME_INC_SASL_H */
