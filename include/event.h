/*
 * Copyright (c) 2005 atheme.org.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Event stuff.
 *
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
  bool active;
};

E struct ev_entry event_table[MAX_EVENTS];
E const char *last_event_ran;

E unsigned int event_add(const char *name, EVH *func, void *arg, time_t when);
E unsigned int event_add_once(const char *name, EVH *func, void *arg,
                          time_t when);
E void event_run(void);
E time_t event_next_time(void);
E void event_delete(EVH *func, void *arg);
E unsigned int event_find(EVH *func, void *arg);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
