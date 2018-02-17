/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Headers for service selection tree.
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
	mowgli_patricia_t *access;

	bool chanmsg;

	mowgli_list_t conf_table;

	bool botonly;

	struct service_ *logtarget;
};

static inline const char *service_get_log_target(const service_t *svs)
{
	if (svs != NULL && svs->logtarget != NULL)
		return service_get_log_target(svs->logtarget);

	return svs != NULL ? svs->nick : me.name;
}

extern mowgli_patricia_t *services_name;
extern mowgli_patricia_t *services_nick;

extern void servtree_init(void);
extern service_t *service_add(const char *name, void (*handler)(sourceinfo_t *si, int parc, char *parv[]));
extern service_t *service_add_static(const char *name, const char *user, const char *host, const char *real, void (*handler)(sourceinfo_t *si, int parc, char *parv[]), service_t *logtarget);
extern void service_delete(service_t *sptr);
extern service_t *service_find_any(void);
extern service_t *service_find(const char *name);
extern service_t *service_find_nick(const char *nick);
extern char *service_name(char *name);
extern void service_set_chanmsg(service_t *, bool);
extern const char *service_resolve_alias(service_t *sptr, const char *context, const char *cmd);
extern const char *service_set_access(service_t *sptr, const char *cmd, const char *access);

extern void service_bind_command(service_t *, command_t *);
extern void service_unbind_command(service_t *, command_t *);

extern void service_named_bind_command(const char *, command_t *);
extern void service_named_unbind_command(const char *, command_t *);
extern void servtree_update(void *dummy);

#endif
