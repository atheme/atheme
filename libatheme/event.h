/*
 * Copyright (c) 2005 atheme.org.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Event stuff.
 *
 * $Id: event.h 3607 2005-11-06 23:57:17Z jilles $
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

E struct ev_entry event_table[MAX_EVENTS];
E const char *last_event_ran;

E uint32_t event_add(const char *name, EVH *func, void *arg, time_t when);
E uint32_t event_add_once(const char *name, EVH *func, void *arg,
                          time_t when);
E void event_run(void);
E time_t event_next_time(void);
E void event_init(void);
E void event_delete(EVH *func, void *arg);
E uint32_t event_find(EVH *func, void *arg);

#endif
