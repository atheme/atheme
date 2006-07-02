/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Commandlist manipulation routines.
 *
 * $Id: commandtree.h 5648 2006-07-02 04:21:38Z nenolod $
 */

#ifndef COMMANDLIST_H
#define COMMANDLIST_H

typedef struct commandentry_ command_t;
typedef struct fcommandentry_ fcommand_t;

struct commandentry_ {
	char *name;
	char *desc;
	const char *access;
	void (*cmd)(char *);
};

struct fcommandentry_ {
	char *name;
	const char *access;
	void (*cmd)(char *, char *);
};

E void command_add(command_t *cmd, list_t *commandtree);
E void command_add_many(command_t **cmd, list_t *commandtree);
E void command_delete(command_t *cmd, list_t *commandtree);
E void command_delete_many(command_t **cmd, list_t *commandtree);
E void command_exec(service_t *svs, char *origin, char *cmd, list_t *commandtree);
E void command_help(char *mynick, char *origin, list_t *commandtree);
E void command_help_short(char *mynick, char *origin, list_t *commandtree, char *maincmds);

E void fcommand_add(fcommand_t *cmd, list_t *commandtree);
E void fcommand_delete(fcommand_t *cmd, list_t *commandtree);
E void fcommand_exec(service_t *svs, char *origin, char *channel, char *cmd, list_t *commandtree);
E void fcommand_exec_floodcheck(service_t *svs, char *origin, char *channel, char *cmd, list_t *commandtree);

#endif
