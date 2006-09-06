/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * Includes most headers usually needed.
 *
 * $Id: atheme.h 6291 2006-09-06 02:26:55Z pippijn $
 */

#ifndef ATHEME_H
#define ATHEME_H

/* *INDENT-OFF* */

#include <org.atheme.claro.base>

#include "common.h"
#include "servers.h"
#include "channels.h"
#include "module.h"
#include "crypto.h"
#include "culture.h"
#include "xmlrpc.h"
#include "base64.h"
#include "md5.h"
#include "sasl.h"
#include "dictionary.h"
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
#include "common-ctcp.h"

#ifdef _WIN32

/* Windows + Module -> needs these to be declared before using them */
#ifdef I_AM_A_MODULE
void _modinit(module_t *m);
void _moddeinit(void);
#endif

#endif

#endif /* ATHEME_H */
