/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for channel information.
 *
 * $Id: channels.h 5514 2006-06-23 15:25:09Z jilles $
 */

#ifndef CHANNELS_H
#define CHANNELS_H

typedef struct user_ user_t; /* XXX: Should be in users.h */
typedef struct channel_ channel_t;
typedef struct chanuser_ chanuser_t;
typedef struct chanban_ chanban_t;

#define MAXEXTMODES 5

struct channel_
{
  char *name;

  uint32_t modes;
  char *key;
  uint32_t limit;
  char *extmodes[MAXEXTMODES]; /* non-standard simple modes with param eg +j */

  uint32_t nummembers;

  time_t ts;
  int32_t hash;

  char *topic;
  char *topic_setter;
  time_t topicts;

  list_t members;
  list_t bans;
};

/* struct for channel users */
struct chanuser_
{
  channel_t *chan;
  user_t *user;
  uint32_t modes;
};

struct chanban_
{
  channel_t *chan;
  char *mask;
  int type; /* 'b', 'e', 'I', etc -- jilles */
};

#define CMODE_OP        0x00000020      /* SPECIAL */
#define CMODE_VOICE     0x00000200      /* SPECIAL */

#define CMODE_INVITE    0x00000001
#define CMODE_KEY       0x00000002
#define CMODE_LIMIT     0x00000004
#define CMODE_MOD       0x00000008
#define CMODE_NOEXT     0x00000010
#define CMODE_PRIV      0x00000040      /* AKA PARA */
#define CMODE_SEC       0x00000080
#define CMODE_TOPIC     0x00000100

#endif
