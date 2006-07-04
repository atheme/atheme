/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for SASL plugin use.
 *
 * $Id: sasl.h 5718 2006-07-04 06:10:05Z gxti $
 */

#ifndef SASL_H
#define SASL_H

typedef struct sasl_session_ sasl_session_t;
typedef struct sasl_message_ sasl_message_t;
typedef struct sasl_mechanism_ sasl_mechanism_t;

struct sasl_session_ {
  char uid[NICKLEN];
  char *buf, *p;
  int len, flags;

  struct sasl_mechanism_ *mechptr;
  void *mechdata;

  char *username;
};

struct sasl_message_ {
  char *uid;
  char mode;
  char *buf;
};

struct sasl_mechanism_ {
  char name[21];
  int (*mech_start) (struct sasl_session_ *sptr, char **buffer, int *buflen);
  int (*mech_step) (struct sasl_session_ *sptr, char *message, int length, char **buffer, int *buflen);
  void (*mech_finish) (struct sasl_session_ *sptr);
};

#define ASASL_FAIL 0 /* client supplied invalid credentials / screwed up their formatting */
#define ASASL_MORE 1 /* everything looks good so far, but we're not done yet */
#define ASASL_DONE 2 /* client successfully authenticated */

#define ASASL_MARKED_FOR_DELETION   1 /* see delete_stale() in saslserv/main.c */

#endif
