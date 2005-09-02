/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures related to services psuedo-clients.
 *
 * $Id: services.h 1033 2005-07-18 22:08:41Z alambert $
 */

#ifndef SERVICES_H
#define SERVICES_H

/* core services */
struct chansvs
{
  char *nick;                   /* the IRC client's nickname  */
  char *user;                   /* the IRC client's username  */
  char *host;                   /* the IRC client's hostname  */
  char *real;                   /* the IRC client's realname  */
  char *disp;			/* the IRC client's dispname  */

  boolean_t fantasy;		/* enable fantasy commands    */

  service_t *me;                /* our user_t struct          */
} chansvs;

struct globsvs
{
  char *nick;
  char *user;
  char *host;
  char *real;
  char *disp;			/* the IRC client's dispname  */
   
  service_t *me;
} globsvs;

struct opersvs
{
  char *nick;
  char *user;
  char *host;
  char *real;
  char *disp;			/* the IRC client's dispname  */
   
  service_t *me;
} opersvs;

/* optional services */
struct nicksvs
{
  boolean_t  spam;

  char   *nick;
  char   *user;
  char   *host;
  char   *real;
  char   *disp;			/* the IRC client's dispname  */

  service_t *me;
} nicksvs;

#endif
