/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2003-2004 E. Will, et al.
 *
 * Includes most headers usually needed.
 */

#include "stdheaders.h"

#ifndef ATHEME_INC_ATHEME_H
#define ATHEME_INC_ATHEME_H 1

#include "abirev.h"
#include "account.h"
#include "atheme_memory.h"
#include "atheme_random.h"
#include "atheme_string.h"
#include "attrs.h"
#include "auth.h"
#include "authcookie.h"
#include "base64.h"
#include "channels.h"
#include "commandhelp.h"
#include "commandtree.h"
#include "common.h"
#include "conf.h"
#include "confprocess.h"
#include "connection.h"
#include "crypto.h"
#include "culture.h"
#include "database_backend.h"
#include "datastream.h"
#include "digest.h"
#include "entity.h"
#include "entity-validation.h"
#include "flags.h"
#include "global.h"
#include "hook.h"
#include "hooktypes.h"
#include "httpd.h"
#include "i18n.h"
#include "inline/account.h"
#include "inline/channels.h"
#include "inline/connection.h"
#include "inline/users.h"
#include "linker.h"
#include "match.h"
#include "module.h"
#include "object.h"
#include "phandler.h"
#include "pmodule.h"
#include "privs.h"
#include "sasl.h"
#include "serno.h"
#include "servers.h"
#include "services.h"
#include "servtree.h"
#include "sharedheap.h"
#include "sourceinfo.h"
#include "structures.h"
#include "table.h"
#include "taint.h"
#include "template.h"
#include "tools.h"
#include "uid.h"
#include "uplink.h"
#include "users.h"

#endif /* !ATHEME_INC_ATHEME_H */
