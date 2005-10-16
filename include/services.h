/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures related to services psuedo-clients.
 *
 * $Id: services.h 2899 2005-10-16 01:22:18Z terminal $
 */

#ifndef SERVICES_H
#define SERVICES_H

typedef struct chansvs_ chansvs_t;
typedef struct globsvs_ globsvs_t;
typedef struct opersvs_ opersvs_t;
typedef struct memosvs_ memosvs_t;
typedef struct helpsvs_ helpsvs_t;
typedef struct nicksvs_ nicksvs_t;
typedef struct usersvs_ usersvs_t;

/* don't forget to define the extern variables below,
   and then the real definitions in atheme.c ! */

/* core services */
struct chansvs_
{
  char *nick;                   /* the IRC client's nickname  */
  char *user;                   /* the IRC client's username  */
  char *host;                   /* the IRC client's hostname  */
  char *real;                   /* the IRC client's realname  */
  char *disp;			/* the IRC client's dispname  */

  boolean_t fantasy;		/* enable fantasy commands    */

  service_t *me;                /* our user_t struct          */
};

struct globsvs_
{
  char *nick;
  char *user;
  char *host;
  char *real;
  char *disp;			/* the IRC client's dispname  */
   
  service_t *me;
};

struct opersvs_
{
  char *nick;
  char *user;
  char *host;
  char *real;
  char *disp;			/* the IRC client's dispname  */
   
  service_t *me;
};

struct memosvs_
{
  char   *nick;
  char   *user;
  char   *host;
  char   *real;
  char   *disp;

  service_t *me;
};

struct helpsvs_
{
  char   *nick;
  char   *user;
  char   *host;
  char   *real;
  char   *disp;

  service_t *me;
};

/* authentication services */
struct nicksvs_
{
  boolean_t  spam;

  char   *nick;
  char   *user;
  char   *host;
  char   *real;
  char   *disp;			/* the IRC client's dispname  */

  service_t *me;
};

struct usersvs_
{
  char   *nick;
  char   *user;
  char   *host;
  char   *real;
  char   *disp;			/* the IRC client's dispname  */

  service_t *me;
};

/* extern declarations */

E chansvs_t chansvs;
E globsvs_t globsvs;
E opersvs_t opersvs;
E memosvs_t memosvs;
E helpsvs_t helpsvs;
E nicksvs_t nicksvs;
E usersvs_t usersvs;

#endif
