/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Commandlist manipulation routines.
 */

#ifndef ATHEME_INC_COMMANDTREE_H
#define ATHEME_INC_COMMANDTREE_H 1

#include <atheme/stdheaders.h>
#include <atheme/structures.h>

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
void command_add(struct command *, mowgli_patricia_t *);
void command_delete(struct command *, mowgli_patricia_t *);
void command_delete_trie_cb(const char *, void *, void *);
struct command *command_find(mowgli_patricia_t *, const char *);
void command_exec(struct service *, struct sourceinfo *, struct command *, int, char **);
void command_exec_split(struct service *, struct sourceinfo *, const char *, char *, mowgli_patricia_t *);
void subcommand_dispatch_simple(struct service *, struct sourceinfo *, int, char **, mowgli_patricia_t *, const char *);
extern bool (*command_authorize)(struct service *, struct sourceinfo *, struct command *c, const char *userlevel);

/* logger.c */
void logaudit_denycmd(struct sourceinfo *si, struct command *cmd, const char *userlevel);

#endif /* !ATHEME_INC_COMMANDTREE_H */
