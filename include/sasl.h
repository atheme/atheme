/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for SASL plugin use.
 *
 */

#ifndef SASL_H
#define SASL_H

#define SASL_MESSAGE_MAXPARA	8	/* arbitrary, increment if needed in future */

typedef struct sasl_session sasl_session_t;
typedef struct sasl_message sasl_message_t;
typedef struct sasl_mechanism sasl_mechanism_t;

struct sasl_session
{
	sasl_mechanism_t  *mechptr;
	server_t          *server;
	char              *uid;
	char              *buf;
	char              *p;
	void              *mechdata;
	char              *username;
	char              *certfp;
	char              *authzid;
	char              *host;
	char              *ip;
	int                len;
	int                flags;
	bool               tls;
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
	char       name[60];
	int      (*mech_start)(sasl_session_t *, char **, size_t *);
	int      (*mech_step)(sasl_session_t *, char *, size_t, char **, size_t *);
	void     (*mech_finish)(sasl_session_t *);
};

typedef struct {

	myuser_t  *source_mu;
	myuser_t  *target_mu;
	bool       allowed;

} hook_sasl_may_impersonate_t;

typedef struct {

	void     (*mech_register)(sasl_mechanism_t *);
	void     (*mech_unregister)(sasl_mechanism_t *);

} sasl_core_functions_t;

#define ASASL_FAIL 0 /* client supplied invalid credentials / screwed up their formatting */
#define ASASL_MORE 1 /* everything looks good so far, but we're not done yet */
#define ASASL_DONE 2 /* client successfully authenticated */

#define ASASL_MARKED_FOR_DELETION   1 /* see delete_stale() in saslserv/main.c */
#define ASASL_NEED_LOG              2 /* user auth success needs to be logged still */

#endif
