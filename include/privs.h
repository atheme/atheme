/*
 * Copyright (C) 2005 Jilles Tjoelker, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Fine grained services operator privileges
 *
 * $Id: privs.h 4419 2006-01-02 12:41:30Z jilles $
 */

#ifndef PRIVS_H
#define PRIVS_H

#define PRIV_NONE            NULL

/* nickserv/userserv */
#define PRIV_USER_AUSPEX     "user:auspex"
#define PRIV_USER_ADMIN      "user:admin"
#define PRIV_USER_VHOST      "user:vhost"
/* chanserv */
#define PRIV_CHAN_AUSPEX     "chan:auspex"
#define PRIV_CHAN_ADMIN      "chan:admin"
#define PRIV_CHAN_CMODES     "chan:cmodes"
#define PRIV_JOIN_STAFFONLY  "chan:joinstaffonly"
/* nickserv/userserv+chanserv */
#define PRIV_MARK            "user:mark"
#define PRIV_HOLD            "user:hold"
#define PRIV_REG_NOLIMIT     "user:regnolimit"
/* generic */
#define PRIV_SERVER_AUSPEX   "general:auspex"
#define PRIV_VIEWPRIVS       "general:viewprivs"
#define PRIV_FLOOD           "general:flood"
#define PRIV_METADATA        "general:metadata"
#define PRIV_ADMIN           "general:admin"
/* operserv */
#define PRIV_OMODE           "operserv:omode"
#define PRIV_AKILL           "operserv:akill"
#define PRIV_JUPE            "operserv:jupe"
#define PRIV_NOOP            "operserv:noop"
#define PRIV_GLOBAL          "operserv:global"
#define PRIV_GRANT           "operserv:grant" /* for (nonexistent) grant privs */

/* obsolete access levels */
#define AC_NONE NULL
/* please do not use the following anymore */
#define AC_IRCOP "special:ircop"
#define AC_SRA "general:admin"
#define is_sra(mu) (has_priv_myuser(mu, PRIV_ADMIN))

/* has_any_privs(): used to determine whether we should give detailed
 * messages about disallowed things
 * warning: do not use this for any kind of real privilege! */
E boolean_t has_any_privs(user_t *);
/* has_priv(): for online users */
E boolean_t has_priv(user_t *, const char *);
/* has_priv_myuser(): channel succession etc */
E boolean_t has_priv_myuser(myuser_t *, const char *);
/* has_priv_operclass(): /os specs etc */
E boolean_t has_priv_operclass(operclass_t *, const char *);

#endif /* PRIVS_H */
