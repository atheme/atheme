/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * Data structures related to our uplink.
 * Modules usually don't need this.
 */

#ifndef ATHEME_INC_UPLINK_H
#define ATHEME_INC_UPLINK_H 1

#include <atheme/attributes.h>
#include <atheme/stdheaders.h>
#include <atheme/structures.h>

struct uplink
{
	mowgli_node_t           node;
	char *                  name;
	char *                  host;
	char *                  send_pass;
	char *                  receive_pass;
	char *                  vhost;
	unsigned int            port;
	struct connection *     conn;
	unsigned int            flags;
};

#define UPF_ILLEGAL 0x80000000U /* not in conf anymore, delete when disconnected */

/* uplink.c */
extern mowgli_list_t uplinks;
extern struct uplink *curr_uplink;

void init_uplinks(void);
struct uplink *uplink_add(const char *name, const char *host, const char *send_password, const char *receive_password, const char *vhost, unsigned int port);
void uplink_delete(struct uplink *u);
struct uplink *uplink_find(const char *name);
void uplink_connect(void);

/* packet.c */
/* bursting timer */
#if HAVE_GETTIMEOFDAY
extern struct timeval burstime;
#endif

extern void (*parse)(char *line);
void irc_handle_connect(struct connection *cptr);

/* send.c */
int sts(const char *fmt, ...) ATHEME_FATTR_PRINTF(1, 2);
void io_loop(void);

#endif /* !ATHEME_INC_UPLINK_H */
