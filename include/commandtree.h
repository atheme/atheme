/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Commandlist manipulation routines.
 */

#ifndef ATHEME_INC_COMMANDTREE_H
#define ATHEME_INC_COMMANDTREE_H 1

struct command
{
	const char *            name;
	const char *            desc;
	const char *            access;
	int                     maxparc;
	void                  (*cmd)(struct sourceinfo *, const int parc, char *parv[]);
	struct {
		const char *    path;
		void          (*func)(struct sourceinfo *, const char *subcmd);
	}                       help;
};

/* commandtree.c */
void command_add(struct command *cmd, mowgli_patricia_t *commandtree);
void command_delete(struct command *cmd, mowgli_patricia_t *commandtree);
struct command *command_find(mowgli_patricia_t *commandtree, const char *command);
void command_exec(struct service *svs, struct sourceinfo *si, struct command *c, int parc, char *parv[]);
void command_exec_split(struct service *svs, struct sourceinfo *si, const char *cmd, char *text, mowgli_patricia_t *commandtree);
void command_help(struct sourceinfo *si, mowgli_patricia_t *commandtree);
void command_help_short(struct sourceinfo *si, mowgli_patricia_t *commandtree, const char *maincmds);
extern bool (*command_authorize)(struct service *svs, struct sourceinfo *si, struct command *c, const char *userlevel);

/* help.c */
void help_display(struct sourceinfo *si, struct service *service, const char *command, mowgli_patricia_t *list);
void help_display_as_subcmd(struct sourceinfo *si, struct service *service, const char *subcmd_of, const char *command, mowgli_patricia_t *list);

/* logger.c */
void logaudit_denycmd(struct sourceinfo *si, struct command *cmd, const char *userlevel);

#endif /* !ATHEME_INC_COMMANDTREE_H */
