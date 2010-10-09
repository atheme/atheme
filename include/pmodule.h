/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Protocol module stuff.
 * Modules usually do not need this.
 *
 */

#ifndef PMODULE_H
#define PMODULE_H

typedef struct pcommand_ pcommand_t;

struct pcommand_ {
	char	*token;
	void	(*handler)(sourceinfo_t *si, int parc, char *parv[]);
	int	minparc;
	int	sourcetype;
};

/* values for sourcetype */
#define MSRC_UNREG	1 /* before SERVER is sent */
#define MSRC_USER	2 /* from users */
#define MSRC_SERVER	4 /* from servers */

#define MAXPARC		35 /* max # params to protocol command */

/* pmodule.c */
E mowgli_heap_t *pcommand_heap;
E mowgli_heap_t *messagetree_heap;
E mowgli_patricia_t *pcommands;

E bool pmodule_loaded;

E void pcommand_init(void);
E void pcommand_add(const char *token,
	void (*handler)(sourceinfo_t *si, int parc, char *parv[]),
	int minparc, int sourcetype);
E void pcommand_delete(const char *token);
E pcommand_t *pcommand_find(const char *token);

/* ptasks.c */
E void handle_version(user_t *);
E void handle_admin(user_t *);
E void handle_info(user_t *);
E void handle_stats(user_t *, char);
E void handle_whois(user_t *, const char *);
E void handle_trace(user_t *, const char *, const char *);
E void handle_motd(user_t *);
E void handle_away(user_t *, const char *);
E void handle_message(sourceinfo_t *, char *, bool, char *);
E void handle_topic_from(sourceinfo_t *, channel_t *, char *, time_t, char *);
E void handle_kill(sourceinfo_t *, const char *, const char *);
E server_t *handle_server(sourceinfo_t *, const char *, const char *, int, const char *);
E void handle_eob(server_t *);
E bool should_reg_umode(user_t *);

/* services.c */
E void services_init(void);
E void reintroduce_user(user_t *u);
E void handle_nickchange(user_t *u);
E void handle_burstlogin(user_t *u, char *login, time_t ts);
E void handle_setlogin(sourceinfo_t *si, user_t *u, char *login, time_t ts);
E void handle_certfp(sourceinfo_t *si, user_t *u, const char *certfp);
E void handle_clearlogin(sourceinfo_t *si, user_t *u);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
