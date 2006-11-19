/*
 * Copyright (C) 2003-2004 E. Will et al.
 * Copyright (C) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Misc tools
 *
 * $Id: tools.h 7233 2006-11-19 19:25:53Z jilles $
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
E int log_force;

/* claro-defined log levels are 0x1F */
/* commands */
#define LG_CMD_ADMIN    0x00000100 /* oper-only commands */
#define LG_CMD_REGISTER 0x00000200 /* register/drop */
#define LG_CMD_SET      0x00000400 /* change properties of static data */
#define LG_CMD_DO       0x00000800 /* change properties of dynamic data */
#define LG_CMD_LOGIN    0x00001000 /* login/logout */
#define LG_CMD_GET      0x00002000 /* query information */
/* other */
#define LG_NETWORK      0x00010000 /* NOTYET netsplit/netjoin */
#define LG_WALLOPS      0x00020000 /* NOTYET wallops from opers/other servers */
#define LG_RAWDATA      0x00040000 /* all data sent/received */

#define LG_CMD_ALL      0x0000FF00
#define LG_ALL          0xFFFFFFFF

/* aliases for use with logcommand() */
#define CMDLOG_ADMIN    LG_CMD_ADMIN
#define CMDLOG_REGISTER LG_CMD_REGISTER
#define CMDLOG_SET      LG_CMD_SET
#define CMDLOG_DO       LG_CMD_DO
#define CMDLOG_LOGIN    LG_CMD_LOGIN
#define CMDLOG_GET      LG_CMD_GET

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
