/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2006 Atheme Project (http://atheme.org/)
 *
 * Stuff for internal use in Atheme.
 */

#ifndef ATHEME_LAC_INTERNAL_H
#define ATHEME_LAC_INTERNAL_H 1

#include <atheme/libathemecore.h>
#include <atheme/stdheaders.h>

/* internal functions */
void event_init(void);
void hooks_init(void);
void init_dlink_nodes(void);
void init_netio(void);
void init_socket_queues(void);
void init_signal_handlers(void);

void language_init(void);

#endif /* !ATHEME_LAC_INTERNAL_H */
