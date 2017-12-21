/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures and macros for SASL mechanisms.
 */

#ifndef SASL_H
#define SASL_H

// Maximum number of parameters for an SASL S2S command (arbitrary, increment in future if necessary)
#define SASL_MESSAGE_MAXPARA        8

// Maximum length of an SASL mechanism name (including terminating NULL byte)
#define SASL_MECHANISM_MAXLEN       60

// Maximum length of Base-64 data a client can send in one shot
#define SASL_S2S_MAXLEN             400

// Maximum length of Base-64 data a client can send in total (buffered)
#define SASL_C2S_MAXLEN             8192

/* Flags for sasl_session->flags
 */
#define ASASL_MARKED_FOR_DELETION   1U  // see delete_stale() in saslserv/main.c
#define ASASL_NEED_LOG              2U  // user auth success needs to be logged still

/* Return values for sasl_mechanism -> mech_start() or mech_step()
 */
#define ASASL_FAIL                  0U  // client supplied invalid credentials / screwed up their formatting
#define ASASL_MORE                  1U  // everything looks good so far, but we're not done yet
#define ASASL_DONE                  2U  // client successfully authenticated
#define ASASL_ERROR                 3U  // an error occurred in mech or it doesn't want to bad_password() the user

struct sasl_session;
struct sasl_message;
struct sasl_mechanism;
struct sasl_core_functions;

struct sasl_session
{
	struct sasl_mechanism   *mechptr;       // Mechanism they're using
	server_t                *server;        // Server they're on
	sourceinfo_t            *si;            // The source info for logcommand(), bad_password(), and login hooks
	char                    *uid;           // Network UID
	char                    *buf;           // Buffered Base-64 data from them (so far)
	void                    *mechdata;      // Mechanism-specific allocated memory
	char                    *authceid;      // Entity ID for authcid
	char                    *authzeid;      // Entity ID for authzid
	char                    *certfp;        // TLS client certificate fingerprint (if any)
	char                    *host;          // Hostname
	char                    *ip;            // IP address
	size_t                   len;           // Length of buffered Base-64 data
	unsigned int             flags;         // Flags (described above)
	bool                     tls;           // Whether their connection to the network is using TLS
};

struct sasl_sourceinfo
{
	sourceinfo_t             parent;
	struct sasl_session     *sess;
};

struct sasl_message
{
	server_t  *server;
	char      *uid;
	char      *parv[SASL_MESSAGE_MAXPARA];
	int        parc;
	char       mode;
};

struct sasl_mechanism
{
	char           name[SASL_MECHANISM_MAXLEN];
	unsigned int (*mech_start)(struct sasl_session *, void **, size_t *);
	unsigned int (*mech_step)(struct sasl_session *, const void *, size_t, void **, size_t *);
	void         (*mech_finish)(struct sasl_session *);
};

struct sasl_core_functions
{
	void     (*mech_register)(struct sasl_mechanism *);
	void     (*mech_unregister)(struct sasl_mechanism *);
	bool     (*authcid_can_login)(struct sasl_session *, const char *, myuser_t **);
	bool     (*authzid_can_login)(struct sasl_session *, const char *, myuser_t **);
};

typedef struct {

	myuser_t  *source_mu;
	myuser_t  *target_mu;
	bool       allowed;

} hook_sasl_may_impersonate_t;

typedef struct sasl_message sasl_message_t;

#endif
