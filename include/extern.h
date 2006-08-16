/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This header file contains all of the extern's needed.
 *
 * $Id: extern.h 6079 2006-08-16 16:44:39Z jilles $
 */

#ifndef EXTERN_H
#define EXTERN_H

/* save some space/typing */
/* -> moved to atheme.h */

E boolean_t cold_start;

/* conf.c */
E boolean_t conf_parse(char *);
E void conf_init(void);
E boolean_t conf_rehash(void);
E boolean_t conf_check(void);

/* confp.c */
E void config_free(CONFIGFILE *cfptr);
E CONFIGFILE *config_load(char *filename);
E CONFIGENTRY *config_find(CONFIGENTRY *ceptr, char *name);

/* function.c */
E FILE *log_file;
E char *log_path;

E char *gen_pw(int8_t sz);

#if HAVE_GETTIMEOFDAY
E void s_time(struct timeval *sttime);
E void e_time(struct timeval sttime, struct timeval *ttime);
E int32_t tv2ms(struct timeval *tv);
#endif

E void tb2sp(char *line);

E void log_open(void);
E void slog(uint32_t level, const char *fmt, ...);
E void logcommand(void *svs, user_t *source, int level, const char *fmt, ...);
E void logcommand_external(void *svs, char *type, connection_t *source, myuser_t *login, int level, const char *fmt, ...);
E uint32_t time_msec(void);
E uint32_t shash(const unsigned char *text);
E char *replace(char *s, int32_t size, const char *old, const char *new);

#ifndef _WIN32
E char *itoa(int num);
#endif

E char *flags_to_string(int32_t flags);
E int32_t mode_to_flag(char c);
E char *time_ago(time_t event);
E char *timediff(time_t seconds);
E unsigned long makekey(void);
E int validemail(char *email);
E boolean_t validhostmask(char *host);
E int sendemail(user_t *from, int type, myuser_t *mu, const char *param);

E boolean_t is_founder(mychan_t *mychan, myuser_t *myuser);
E boolean_t is_xop(mychan_t *mychan, myuser_t *myuser, uint32_t level);
E boolean_t should_owner(mychan_t *mychan, myuser_t *myuser);
E boolean_t should_protect(mychan_t *mychan, myuser_t *myuser);
E boolean_t is_soper(myuser_t *myuser);
E boolean_t is_ircop(user_t *user);
E boolean_t is_admin(user_t *user);
E boolean_t is_internal_client(user_t *user);

E void set_password(myuser_t *mu, char *newpassword);
E boolean_t verify_password(myuser_t *mu, char *password);

E int token_to_value(struct Token token_table[], char *token);
E char *sbytes(float x);
E float bytes(float x);

E helpentry_t *help_cmd_find(char *svs, char *origin, char *command,
			     list_t *list);

/* node.c */
E void init_nodes(void);
/* The following currently only do uplinks -- jilles */
E void mark_all_illegal(void);
E void unmark_all_illegal(void);
E void remove_illegals(void);

/* atheme.c */
E char *config_file;

/* uid.c */
E void init_uid(void);
E char *uid_get(void);
E void add_one_to_uid(uint32_t i);

/* tokenize.c */
E int8_t sjtoken(char *message, char delimiter, char **parv);
E int8_t tokenize(char *message, char **parv);

/* version.c */
E const char *generation;
E const char *creation;
E const char *platform;
E const char *version;
E const char *revision;
E const char *osinfo;
E const char *infotext[];

/* ubase64.c */
E const char* uinttobase64(char* buf, uint64_t v, int64_t count);
E uint32_t base64touint(char* buf);

/* signal.c */
E void sighandler(int signum);
E void check_signals(void);

/* help.c */
E void help_display(char *svsnick, char *svsdisp, char *origin, char *command, list_t *list);
E void help_addentry(list_t *list, char *topic, char *fname,
	void (*func)(char *origin));
E void help_delentry(list_t *list, char *name);

#endif /* EXTERN_H */
