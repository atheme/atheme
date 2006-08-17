/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This header file contains all of the extern's needed.
 *
 * $Id: extern.h 6093 2006-08-17 15:36:43Z jilles $
 */

#ifndef EXTERN_H
#define EXTERN_H

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

E char *time_ago(time_t event);
E char *timediff(time_t seconds);
E unsigned long makekey(void);
E int validemail(char *email);
E boolean_t validhostmask(char *host);
E int sendemail(user_t *from, int type, myuser_t *mu, const char *param);

E char *sbytes(float x);
E float bytes(float x);

/* node.c */
E void init_nodes(void);
/* The following currently only do uplinks -- jilles */
E void mark_all_illegal(void);
E void unmark_all_illegal(void);
E void remove_illegals(void);

/* atheme.c */
E boolean_t cold_start;
E char *config_file;

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

#endif /* EXTERN_H */
