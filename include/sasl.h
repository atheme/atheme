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

typedef struct sasl_session_ sasl_session_t;
typedef struct sasl_message_ sasl_message_t;
typedef struct sasl_mechanism_ sasl_mechanism_t;

struct sasl_session_ {
  char *uid;
  char *buf, *p;
  int len, flags;

  server_t *server;

  struct sasl_mechanism_ *mechptr;
  void *mechdata;

  char *username;
  char *certfp;
  char *authzid;

  char *host;
  char *ip;
  bool tls;
};

struct sasl_message_ {
  char *uid;
  char mode;
  char *parv[SASL_MESSAGE_MAXPARA];
  int parc;

  server_t *server;
};

struct sasl_mechanism_ {
  char name[60];
  int (*mech_start) (struct sasl_session_ *sptr, char **buffer, size_t *buflen);
  int (*mech_step) (struct sasl_session_ *sptr, char *message, size_t length, char **buffer, size_t *buflen);
  void (*mech_finish) (struct sasl_session_ *sptr);
};

typedef struct {
  myuser_t *source_mu;
  myuser_t *target_mu;
  bool allowed;
} hook_sasl_may_impersonate_t;

typedef struct {
	void (*mech_register) (struct sasl_mechanism_ *mech);
	void (*mech_unregister) (struct sasl_mechanism_ *mech);
} sasl_mech_register_func_t;

#define ASASL_FAIL 0 /* client supplied invalid credentials / screwed up their formatting */
#define ASASL_MORE 1 /* everything looks good so far, but we're not done yet */
#define ASASL_DONE 2 /* client successfully authenticated */

#define ASASL_MARKED_FOR_DELETION   1 /* see delete_stale() in saslserv/main.c */
#define ASASL_NEED_LOG              2 /* user auth success needs to be logged still */

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
