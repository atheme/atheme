/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for SASL plugin use.
 *
 */

#ifndef SASL_H
#define SASL_H

#define SASL_MESSAGE_MAXPARA    8       /* arbitrary, increment if needed in future */
#define SASL_MECHANISM_MAXLEN   60
#define SASL_S2S_MAXLEN         400

struct sasl_session;
struct sasl_message;
struct sasl_mechanism;
struct sasl_core_functions;

struct sasl_session
{
	struct sasl_mechanism   *mechptr;
	server_t                *server;
	sourceinfo_t            *si;
	char                    *uid;
	char                    *buf;
	char                    *p;
	void                    *mechdata;
	char                    *authceid;
	char                    *authzeid;
	char                    *certfp;
	char                    *host;
	char                    *ip;
	size_t                   len;
	int                      flags;
	bool                     tls;
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
	char       name[SASL_MECHANISM_MAXLEN];
	int      (*mech_start)(struct sasl_session *, char **, size_t *);
	int      (*mech_step)(struct sasl_session *, char *, size_t, char **, size_t *);
	void     (*mech_finish)(struct sasl_session *);
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

#define ASASL_FAIL 0 /* client supplied invalid credentials / screwed up their formatting */
#define ASASL_MORE 1 /* everything looks good so far, but we're not done yet */
#define ASASL_DONE 2 /* client successfully authenticated */

#define ASASL_MARKED_FOR_DELETION   1 /* see delete_stale() in saslserv/main.c */
#define ASASL_NEED_LOG              2 /* user auth success needs to be logged still */

#endif
