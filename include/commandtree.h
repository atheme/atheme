/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Commandlist manipulation routines.
 *
 */

#ifndef COMMANDLIST_H
#define COMMANDLIST_H

typedef struct commandentry_ command_t;

struct commandentry_ {
	const char *name;
	const char *desc;
	const char *access;
        const int maxparc;
	void (*cmd)(sourceinfo_t *, const int parc, char *parv[]);
	struct {
		const char *path;
		void (*func)(sourceinfo_t *, const char *subcmd);
	} help;
};

/* commandtree.c */
E void command_add(command_t *cmd, mowgli_patricia_t *commandtree);
E void command_delete(command_t *cmd, mowgli_patricia_t *commandtree);
E command_t *command_find(mowgli_patricia_t *commandtree, const char *command);
E void command_exec(service_t *svs, sourceinfo_t *si, command_t *c, int parc, char *parv[]);
E void command_exec_split(service_t *svs, sourceinfo_t *si, const char *cmd, char *text, mowgli_patricia_t *commandtree);
E void command_help(sourceinfo_t *si, mowgli_patricia_t *commandtree);
E void command_help_short(sourceinfo_t *si, mowgli_patricia_t *commandtree, const char *maincmds);
E bool (*command_authorize)(service_t *svs, sourceinfo_t *si, command_t *c, const char *userlevel);

/* help.c */
E void help_display(sourceinfo_t *si, service_t *service, const char *command, mowgli_patricia_t *list);

/* logger.c */
E void logaudit_denycmd(sourceinfo_t *si, command_t *cmd, const char *userlevel);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
