/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Commandlist manipulation routines.
 *
 * $Id: commandtree.h 6727 2006-10-20 18:48:53Z jilles $
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
};

/* struct for help command hash table */
struct help_command_
{
  char *name;
  const char *access;
  char *file;
  void (*func)(sourceinfo_t *si);
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
E void command_help(sourceinfo_t *si, list_t *commandtree);
E void command_help_short(sourceinfo_t *si, list_t *commandtree, char *maincmds);

/* help.c */
E void help_display(sourceinfo_t *si, char *command, list_t *list);
E void help_addentry(list_t *list, char *topic, char *fname,
	void (*func)(sourceinfo_t *si));
E void help_delentry(list_t *list, char *name);

#endif
