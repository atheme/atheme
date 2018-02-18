/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Commandlist manipulation routines.
 */

#ifndef COMMANDLIST_H
#define COMMANDLIST_H

struct command
{
	const char *name;
	const char *desc;
	const char *access;
        int maxparc;
	void (*cmd)(sourceinfo_t *, const int parc, char *parv[]);
	struct {
		const char *path;
		void (*func)(sourceinfo_t *, const char *subcmd);
	} help;
};

/* commandtree.c */
extern void command_add(struct command *cmd, mowgli_patricia_t *commandtree);
extern void command_delete(struct command *cmd, mowgli_patricia_t *commandtree);
extern struct command *command_find(mowgli_patricia_t *commandtree, const char *command);
extern void command_exec(struct service *svs, sourceinfo_t *si, struct command *c, int parc, char *parv[]);
extern void command_exec_split(struct service *svs, sourceinfo_t *si, const char *cmd, char *text, mowgli_patricia_t *commandtree);
extern void command_help(sourceinfo_t *si, mowgli_patricia_t *commandtree);
extern void command_help_short(sourceinfo_t *si, mowgli_patricia_t *commandtree, const char *maincmds);
extern bool (*command_authorize)(struct service *svs, sourceinfo_t *si, struct command *c, const char *userlevel);

/* help.c */
extern void help_display(sourceinfo_t *si, struct service *service, const char *command, mowgli_patricia_t *list);
extern void help_display_as_subcmd(sourceinfo_t *si, struct service *service, const char *subcmd_of, const char *command, mowgli_patricia_t *list);

/* logger.c */
extern void logaudit_denycmd(sourceinfo_t *si, struct command *cmd, const char *userlevel);

#endif
