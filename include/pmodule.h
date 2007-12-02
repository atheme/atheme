/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Protocol module stuff.
 * Modules usually do not need this.
 *
 * $Id: pmodule.h 8263 2007-05-17 22:31:56Z jilles $
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
E BlockHeap *pcommand_heap;
E BlockHeap *messagetree_heap;
E mowgli_dictionary_t *pcommands;

E boolean_t pmodule_loaded;

E void pcommand_init(void);
E void pcommand_add(char *token,
	void (*handler)(sourceinfo_t *si, int parc, char *parv[]),
	int minparc, int sourcetype);
E void pcommand_delete(char *token);
E pcommand_t *pcommand_find(char *token);

/* ptasks.c */
E void handle_version(user_t *);
E void handle_admin(user_t *);
E void handle_info(user_t *);
E void handle_stats(user_t *, char);
E void handle_whois(user_t *, const char *);
E void handle_trace(user_t *, const char *, const char *);
E void handle_motd(user_t *);
E void handle_away(user_t *, const char *);
E void handle_message(sourceinfo_t *, char *, boolean_t, char *);
E void handle_topic_from(sourceinfo_t *, channel_t *, char *, time_t, char *);
E void handle_kill(sourceinfo_t *, const char *, const char *);
E server_t *handle_server(sourceinfo_t *, const char *, const char *, int, const char *);
E void handle_eob(server_t *);
E boolean_t should_reg_umode(user_t *);

/* services.c */
E void services_init(void);
E void reintroduce_user(user_t *u);
E void handle_nickchange(user_t *u);
E void handle_burstlogin(user_t *u, char *login);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
