/* cmd.h - command dispatching */

#ifndef CMD_H
#define CMD_H

typedef struct cmd_ {
	const char *desc;
	const char *access;
	const char *help;
	int maxparc;
	void (*func)(sourceinfo_t *si, int parc, char *parv[]);
} cmd_t;

/* These functions deal entirely in canonical names for commands. The functions
 * below implement local aliases for particular services or chunks of command
 * trees. There is only one command map, which stores all commands. */
E void cmd_init(void);
E void cmd_add(const char *name, cmd_t *cmd);
E void cmd_del(const char *name);
E cmd_t *cmd_find(const char *name);
E void cmd_split(cmd_t *cmd, char *buf, int *parc, char **parv);
E void cmd_exec(sourceinfo_t *si, cmd_t *cmd, int parc, char *parv[]);
E void cmd_help(sourceinfo_t *si, cmd_t *cmd);
E void cmd_help_short(sourceinfo_t *si, cmd_t *cmd, const char *maincmds);

typedef struct cmdmap_ {
	mowgli_patricia_t *tree;
} cmdmap_t;

/* These functions deal in name -> canonical name mapping. These mappings are
 * per-service (or sometimes local to a submodule, as in e.g. CS SET). Aliases
 * are also done with these. */
E cmdmap_t *cmdmap_new(void);
E void cmdmap_init(cmdmap_t *map);
E void cmdmap_destroy(cmdmap_t *map);
E void cmdmap_free(cmdmap_t *map);

E void cmdmap_add(cmdmap_t *map, const char *name, const char *cname);
E void cmdmap_del(cmdmap_t *map, const char *name);
E const char *cmdmap_resolve(cmdmap_t *map, const char *name);
E cmd_t *cmdmap_get(cmdmap_t *map, const char *name);

#endif /* !CMD_H */
