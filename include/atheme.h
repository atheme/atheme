/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * Includes most headers usually needed.
 *
 * $Id: atheme.h 7837 2007-03-06 00:06:49Z nenolod $
 */

#ifndef ATHEME_H
#define ATHEME_H

/* *INDENT-OFF* */

#ifdef _WIN32                   /* Windows */
#       ifdef I_AM_A_MODULE
#               define DLE __declspec (dllimport)
#               define E extern DLE
#       else
#               define DLE __declspec (dllexport)
#               define E extern DLE
#       endif
#else                           /* POSIX */
#       define E extern
#       define DLE
#endif

#include "sysconf.h"
#include "i18n.h"
#include "common.h"
#include "dlink.h"
#include "object.h"
#include "event.h"
#include "balloc.h"
#include "connection.h"
#include "sockio.h"
#include "hook.h"
#include "linker.h"
#include "atheme_string.h"
#include "atheme_memory.h"
#include "datastream.h"
#include "dictionary.h"
#include "servers.h"
#include "channels.h"
#include "module.h"
#include "crypto.h"
#include "culture.h"
#include "xmlrpc.h"
#include "base64.h"
#include "md5.h"
#include "sasl.h"
#include "match.h"
#include "sysconf.h"
#include "account.h"
#include "tools.h"
#include "confparse.h"
#include "global.h"
#include "flags.h"
#include "metadata.h"
#include "phandler.h"
#include "servtree.h"
#include "services.h"
#include "commandtree.h"
#include "users.h"
#include "sourceinfo.h"
#include "authcookie.h"
#include "privs.h"

#ifdef _WIN32

/* Windows + Module -> needs these to be declared before using them */
#ifdef I_AM_A_MODULE
void _modinit(module_t *m);
void _moddeinit(void);
#endif

/* Windows has an extremely stupid gethostbyname() function. Oof! */
#define gethostbyname(a) gethostbyname_layer(a)

#endif

#endif /* ATHEME_H */

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
