/*
 * Copyright (C) 2003-2004 E. Will et al.
 * Copyright (C) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Misc tools
 *
 * $Id: tools.h 6895 2006-10-22 21:07:24Z jilles $
 */

#ifndef _TOOLS_H
#define _TOOLS_H

/* email stuff */
/* the following struct is not used yet */
typedef struct email_t_ email_t;
struct email_t_
{
	char *sender;
	char *reciever;
	char *subject;
	char *body;
	char **headers;
	
	void *miscellaneous;			/* module defined data */
	void (*callback_sent)(email_t *);	/* callback on email send */
};

E int sendemail(user_t *from, int type, myuser_t *mu, const char *param);

/* email types (meaning of param argument) */
#define EMAIL_REGISTER 1 /* register an account/nick (verification code) */
#define EMAIL_SENDPASS 2 /* send a password to a user (password) */
#define EMAIL_SETEMAIL 3 /* change email address (verification code) */
#define EMAIL_MEMO     4 /* emailed memos (memo text) */

/* function.c */
/* logging stuff */
E FILE *log_file;
E char *log_path;

/* command log levels */
#define CMDLOG_ADMIN    1 /* oper-only commands */
#define CMDLOG_REGISTER 2 /* register/drop */
#define CMDLOG_SET      3 /* change properties of static data */
#define CMDLOG_DO       4 /* change properties of dynamic data */
#define CMDLOG_LOGIN    5 /* login/logout */
#define CMDLOG_GET      6 /* query information */

E void log_open(void);
E void slog(uint32_t level, const char *fmt, ...);
E void logcommand(sourceinfo_t *si, int level, const char *fmt, ...);
E void logcommand_user(service_t *svs, user_t *source, int level, const char *fmt, ...);
E void logcommand_external(service_t *svs, const char *type, connection_t *source, const char *sourcedesc, myuser_t *login, int level, const char *fmt, ...);

/* misc string stuff */
E char *gen_pw(int8_t sz);
E void tb2sp(char *line);
E char *replace(char *s, int32_t size, const char *old, const char *new);
#ifndef _WIN32
E char *itoa(int num);
#endif
E int validemail(char *email);
E boolean_t validhostmask(char *host);
E char *sbytes(float x);
E float bytes(float x);

E unsigned long makekey(void);

/* the hash function */
E uint32_t shash(const unsigned char *text);

/* time stuff */
#if HAVE_GETTIMEOFDAY
E void s_time(struct timeval *sttime);
E void e_time(struct timeval sttime, struct timeval *ttime);
E int32_t tv2ms(struct timeval *tv);
#endif
E char *time_ago(time_t event);
E char *timediff(time_t seconds);

#ifndef timersub
#define timersub(tvp, uvp, vvp)                                         \
        do {                                                            \
                (vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;          \
                (vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;       \
                if ((vvp)->tv_usec < 0) {                               \
                        (vvp)->tv_sec--;                                \
                        (vvp)->tv_usec += 1000000;                      \
                }                                                       \
        } while (0)
#endif

/* tokenize.c */
E int8_t sjtoken(char *message, char delimiter, char **parv);
E int8_t tokenize(char *message, char **parv);

/* ubase64.c */
E const char* uinttobase64(char* buf, uint64_t v, int64_t count);
E uint32_t base64touint(char* buf);

#endif
