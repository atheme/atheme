/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Headers for service selection tree.
 *
 */

#ifndef SERVTREE_H
#define SERVTREE_H

struct service_ {
	char *internal_name;

	char *nick;
	char *user;
	char *host;
	char *real;
	char *disp;

	user_t *me;

	void (*handler)(sourceinfo_t *, int, char **);
	void (*notice_handler)(sourceinfo_t *, int, char **);

	mowgli_patricia_t *commands;
	mowgli_patricia_t *aliases;

	bool chanmsg;

	mowgli_list_t *conf_table;
};

E mowgli_patricia_t *services_name;
E mowgli_patricia_t *services_nick;

E void servtree_init(void);
E service_t *service_add(const char *name, void (*handler)(sourceinfo_t *si, int parc, char *parv[]), mowgli_list_t *conf_table);
E service_t *service_add_static(const char *name, const char *user, const char *host, const char *real, void (*handler)(sourceinfo_t *si, int parc, char *parv[]));
E void service_delete(service_t *sptr);
E service_t *service_find(const char *name);
E service_t *service_find_nick(const char *nick);
E char *service_name(char *name);
E void service_set_chanmsg(service_t *, bool);
E const char *service_resolve_alias(service_t *sptr, const char *context, const char *cmd);

E void service_bind_command(service_t *, command_t *);
E void service_unbind_command(service_t *, command_t *);

E void service_named_bind_command(const char *, command_t *);
E void service_named_unbind_command(const char *, command_t *);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
