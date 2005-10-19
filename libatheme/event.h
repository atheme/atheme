/*
 * Copyright (c) 2005 atheme.org.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Event stuff.
 *
 * $Id$
 */

#ifndef EVENT_H
#define EVENT_H

typedef void EVH(void *);

/* event list struct */
struct ev_entry
{
  EVH *func;
  void *arg;
  const char *name;
  time_t frequency;
  time_t when;
  boolean_t active;
};

#endif
