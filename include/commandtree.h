/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Commandlist manipulation routines.
 *
 * $Id: commandtree.h 6585 2006-09-30 22:10:34Z jilles $
 */

#ifndef COMMANDLIST_H
#define COMMANDLIST_H

typedef struct commandentry_ command_t;
typedef struct fcommandentry_ fcommand_t;

struct commandentry_ {
	const char *name;
	const char *desc;
	const char *access;
        const int maxparc;
	void (*cmd)(sourceinfo_t *, const int parc, char *parv[]);
};

struct fcommandentry_ {
	const char *name;
	const char *access;
	void (*cmd)(char *, char *);
};

/* struct for help command hash table */
struct help_command_
{
  char *name;
  const char *access;
  char *file;
  void (*func) (char *origin);
};
typedef struct help_command_ helpentry_t;

/* commandtree.c */
E void command_add(command_t *cmd, list_t *commandtree);
E void command_add_many(command_t **cmd, list_t *commandtree);
E void command_delete(command_t *cmd, list_t *commandtree);
E void command_delete_many(command_t **cmd, list_t *commandtree);
E command_t *command_find(list_t *commandtree, const char *command);
E void command_exec(service_t *svs, sourceinfo_t *si, command_t *c, int parc, char *parv[]);
E void command_exec_split(service_t *svs, sourceinfo_t *si, char *cmd, char *text, list_t *commandtree);
E void command_help(char *mynick, char *origin, list_t *commandtree);
E void command_help_short(char *mynick, char *origin, list_t *commandtree, char *maincmds);

E void fcommand_add(fcommand_t *cmd, list_t *commandtree);
E void fcommand_delete(fcommand_t *cmd, list_t *commandtree);
E void fcommand_exec(service_t *svs, char *channel, char *origin, char *cmd, list_t *commandtree);
E void fcommand_exec_floodcheck(service_t *svs, char *channel, char *origin, char *cmd, list_t *commandtree);

/* help.c */
E void help_display(sourceinfo_t *si, char *command, list_t *list);
E void help_addentry(list_t *list, char *topic, char *fname,
	void (*func)(char *origin));
E void help_delentry(list_t *list, char *name);

#endif
