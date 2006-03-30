/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for SASL plugin use.
 *
 * $Id: sasl.h 4935 2006-03-30 16:13:33Z nenolod $
 */

#ifndef SASL_H
#define SASL_H

#include <gsasl.h>

typedef struct sasl_session_ sasl_session_t;
typedef struct sasl_message_ sasl_message_t;

struct sasl_session_ {
  char uid[NICKLEN];
  char mech[21];
  char *buf, *p;
  int len;

  Gsasl_session *sctx;
};

struct sasl_message_ {
  char *uid;
  char mode;
  char *buf;
};

#endif
