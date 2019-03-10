/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * Protocol module stuff.
 * Modules usually do not need this.
 */

#ifndef ATHEME_INC_PMODULE_H
#define ATHEME_INC_PMODULE_H 1

#include <atheme/sourceinfo.h>
#include <atheme/stdheaders.h>

struct proto_cmd
{
	char *  token;
	void  (*handler)(struct sourceinfo *si, int parc, char *parv[]);
	int     minparc;
	int     sourcetype;
};

/* values for sourcetype */
#define MSRC_UNREG	1 /* before SERVER is sent */
#define MSRC_USER	2 /* from users */
#define MSRC_SERVER	4 /* from servers */

#define MAXPARC		35 /* max # params to protocol command */

/* pmodule.c */
extern mowgli_heap_t *pcommand_heap;
extern mowgli_heap_t *messagetree_heap;
extern mowgli_patricia_t *pcommands;

void pcommand_init(void);
void pcommand_add(const char *token,
	void (*handler)(struct sourceinfo *si, int parc, char *parv[]),
	int minparc, int sourcetype);
void pcommand_delete(const char *token);
struct proto_cmd *pcommand_find(const char *token);

/* ptasks.c */
const char *get_build_date(void);
int get_version_string(char *, size_t);
void handle_version(struct user *);
void handle_admin(struct user *);
void handle_info(struct user *);
void handle_stats(struct user *, char);
void handle_whois(struct user *, const char *);
void handle_trace(struct user *, const char *, const char *);
void handle_motd(struct user *);
void handle_away(struct user *, const char *);
void handle_message(struct sourceinfo *, char *, bool, char *);
void handle_topic_from(struct sourceinfo *, struct channel *, const char *, time_t, const char *);
void handle_kill(struct sourceinfo *, const char *, const char *);
struct server *handle_server(struct sourceinfo *, const char *, const char *, int, const char *);
void handle_eob(struct server *);
bool should_reg_umode(struct user *);

/* services.c */
void services_init(void);
void reintroduce_user(struct user *u);
void handle_nickchange(struct user *u);
void handle_burstlogin(struct user *u, const char *login, time_t ts);
void handle_setlogin(struct sourceinfo *si, struct user *u, const char *login, time_t ts);
void handle_certfp(struct sourceinfo *si, struct user *u, const char *certfp);
void handle_clearlogin(struct sourceinfo *si, struct user *u);

#endif /* !ATHEME_INC_PMODULE_H */
