/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2003-2004 E. Will, et al.
 * Copyright (C) 2005-2006 Atheme Project (http://atheme.org/)
 *
 * Misc tools
 */

#ifndef ATHEME_INC_TOOLS_H
#define ATHEME_INC_TOOLS_H 1

#include <atheme/attributes.h>
#include <atheme/common.h>
#include <atheme/constants.h>
#include <atheme/object.h>
#include <atheme/stdheaders.h>
#include <atheme/structures.h>

/* email stuff */
int sendemail(struct user *u, struct myuser *mu, const char *type, const char *email, const char *param);

/* email types (meaning of param argument) */
#define EMAIL_REGISTER	"register"	/* register an account/nick (verification code) */
#define EMAIL_SENDPASS	"sendpass"	/* send a password to a user (password) */
#define EMAIL_SETEMAIL	"setemail"	/* change email address (verification code) */
#define EMAIL_MEMO	"memo"		/* emailed memos (memo text) */
#define EMAIL_SETPASS	"setpass"	/* send a password change key (verification code) */

/* cidr.c */
int valid_ip_or_mask(const char *src);

enum log_type
{
	LOG_ANY = 0,
	LOG_INTERACTIVE = 1, /* IRC channels */
	LOG_NONINTERACTIVE = 2 /* files */
};

/* logstreams API --nenolod */
typedef void (*log_write_func_fn)(struct logfile *lf, const char *buf);

/* logger.c */
struct logfile
{
	struct atheme_object    parent;
	mowgli_node_t           node;
	void *                  log_file;       // opaque: can either be struct mychan or FILE --nenolod
	char *                  log_path;
	unsigned int            log_mask;
	log_write_func_fn       write_func;
	enum log_type           log_type;
};

extern char *log_path; /* contains path to default log. */
extern int log_force;

struct logfile *logfile_new(const char *log_path_, unsigned int log_mask) ATHEME_FATTR_MALLOC;
void logfile_register(struct logfile *lf);
void logfile_unregister(struct logfile *lf);

/* general */
#define LG_NONE         0x00000001U      /* don't log                */
#define LG_INFO         0x00000002U      /* log general info         */
#define LG_ERROR        0x00000004U      /* log real important stuff */
#define LG_IOERROR      0x00000008U      /* log I/O errors. */
#define LG_DEBUG        0x00000010U      /* log debugging stuff      */
#define LG_VERBOSE	0x00000020U	/* log a bit more verbosely than INFO or REGISTER, but not as much as DEBUG */
/* commands */
#define LG_CMD_ADMIN    0x00000100U /* oper-only commands */
#define LG_CMD_REGISTER 0x00000200U /* register/drop */
#define LG_CMD_SET      0x00000400U /* change properties of static data */
#define LG_CMD_DO       0x00000800U /* change properties of dynamic data */
#define LG_CMD_LOGIN    0x00001000U /* login/logout */
#define LG_CMD_GET      0x00002000U /* query information */
#define LG_CMD_REQUEST	0x00004000U /* requests made by users */
/* other */
#define LG_NETWORK      0x00010000U /* netsplit/netjoin */
#define LG_WALLOPS      0x00020000U /* NOTYET wallops from opers/other servers */
#define LG_RAWDATA      0x00040000U /* all data sent/received */
#define LG_REGISTER     0x00080000U /* all registration related messages */
#define LG_WARN1        0x00100000U /* NOTYET messages formerly walloped */
#define LG_WARN2        0x00200000U /* NOTYET messages formerly snooped */
#define LG_DENYCMD	0x00400000U /* commands denied by security policy */

#define LG_CMD_ALL      0x0000FF00U
#define LG_ALL          0x7FFFFFFFU /* XXX cannot use bit 31 as it would then be equal to TOKEN_UNMATCHED */

/* aliases for use with logcommand() */
#define CMDLOG_ADMIN    LG_CMD_ADMIN
#define CMDLOG_REGISTER (LG_CMD_REGISTER | LG_REGISTER)
#define CMDLOG_SET      LG_CMD_SET
#define CMDLOG_REQUEST	LG_CMD_REQUEST
#define CMDLOG_DO       LG_CMD_DO
#define CMDLOG_LOGIN    LG_CMD_LOGIN
#define CMDLOG_GET      LG_CMD_GET

void log_open(void);
void log_shutdown(void);
bool log_debug_enabled(void);
void log_master_set_mask(unsigned int mask);
struct logfile *logfile_find_mask(unsigned int log_mask);
void slog(unsigned int level, const char *fmt, ...) ATHEME_FATTR_PRINTF(2, 3);
void logcommand(struct sourceinfo *si, int level, const char *fmt, ...) ATHEME_FATTR_PRINTF(3, 4);
void logcommand_user(struct service *svs, struct user *source, int level, const char *fmt, ...) ATHEME_FATTR_PRINTF(4, 5);
void logcommand_external(struct service *svs, const char *type, struct connection *source, const char *sourcedesc, struct myuser *login, int level, const char *fmt, ...) ATHEME_FATTR_PRINTF(7, 8);

/* function.c */

typedef void (*email_canonicalizer_fn)(char email[static (EMAILLEN + 1)], void *user_data);

struct email_canonicalizer_item
{
	email_canonicalizer_fn  func;
	void *                  user_data;
	mowgli_node_t           node;
};

/* misc string stuff */
bool string_in_list(const char *str, const char *list);
char *random_string(size_t sz) ATHEME_FATTR_MALLOC ATHEME_FATTR_RETURNS_NONNULL;
const char *create_weak_challenge(struct sourceinfo *si, const char *name);
void tb2sp(char *line);
char *replace(char *s, int size, const char *old, const char *new);
const char *number_to_string(int num);
bool string_to_uint(const char *, unsigned int *) ATHEME_FATTR_WUR;
int validemail(const char *email);
stringref canonicalize_email(const char *email);
void canonicalize_email_case(char email[static (EMAILLEN + 1)], void *user_data);
void register_email_canonicalizer(email_canonicalizer_fn func, void *user_data);
void unregister_email_canonicalizer(email_canonicalizer_fn func, void *user_data);
bool email_within_limits(const char *email);
bool validhostmask(const char *host);
char *pretty_mask(char *mask);
bool validtopic(const char *topic);
bool has_ctrl_chars(const char *text);
char *sbytes(float x);
float bytes(float x);
char *combine_path(const char *parent, const char *child);

unsigned long makekey(void);
int srename(const char *old_fn, const char *new_fn);

/* time stuff */
#if HAVE_GETTIMEOFDAY
void s_time(struct timeval *sttime);
void e_time(struct timeval sttime, struct timeval *ttime);
int tv2ms(struct timeval *tv);
#endif
char *time_ago(time_t event);
char *timediff(time_t seconds);

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
int sjtoken(char *message, char delimiter, char **parv);
int tokenize(char *message, char **parv);

/* ubase64.c */
const char *uinttobase64(char *buf, uint64_t v, int64_t count);
unsigned int base64touint(const char *buf);
void decode_p10_ip(const char *b64, char ipstring[HOSTIPLEN + 1]);

#endif /* !ATHEME_INC_TOOLS_H */
