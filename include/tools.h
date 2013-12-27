/*
 * Copyright (C) 2003-2004 E. Will et al.
 * Copyright (C) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Misc tools
 *
 */

#ifndef ATHEME_TOOLS_H
#define ATHEME_TOOLS_H

/* email stuff */
E int sendemail(user_t *u, myuser_t *mu, const char *type, const char *email, const char *param);

/* email types (meaning of param argument) */
#define EMAIL_REGISTER	"register"	/* register an account/nick (verification code) */
#define EMAIL_SENDPASS	"sendpass"	/* send a password to a user (password) */
#define EMAIL_SETEMAIL	"setemail"	/* change email address (verification code) */
#define EMAIL_MEMO	"memo"		/* emailed memos (memo text) */
#define EMAIL_SETPASS	"setpass"	/* send a password change key (verification code) */

/* arc4random.c */
#ifndef HAVE_ARC4RANDOM
E void arc4random_stir(void);
E void arc4random_addrandom(unsigned char *dat, int datlen);
E unsigned int arc4random(void);
#endif /* !HAVE_ARC4RANDOM */

typedef enum {
	LOG_ANY = 0,
	LOG_INTERACTIVE = 1, /* IRC channels */
	LOG_NONINTERACTIVE = 2 /* files */
} log_type_t;

typedef struct logfile_ logfile_t;

/* logstreams API --nenolod */
typedef void (*log_write_func_t)(logfile_t *lf, const char *buf);

/* logger.c */
struct logfile_ {
	object_t parent;
	mowgli_node_t node;

	void *log_file;		/* opaque: can either be mychan_t or FILE --nenolod */
	char *log_path;
	unsigned int log_mask;

	log_write_func_t write_func;
	log_type_t log_type;
};

E char *log_path; /* contains path to default log. */
E int log_force;

E logfile_t *logfile_new(const char *log_path_, unsigned int log_mask);
E void logfile_register(logfile_t *lf);
E void logfile_unregister(logfile_t *lf);

/* general */
#define LG_NONE         0x00000001      /* don't log                */
#define LG_INFO         0x00000002      /* log general info         */
#define LG_ERROR        0x00000004      /* log real important stuff */
#define LG_IOERROR      0x00000008      /* log I/O errors. */
#define LG_DEBUG        0x00000010      /* log debugging stuff      */
#define LG_VERBOSE	0x00000020	/* log a bit more verbosely than INFO or REGISTER, but not as much as DEBUG */
/* commands */
#define LG_CMD_ADMIN    0x00000100 /* oper-only commands */
#define LG_CMD_REGISTER 0x00000200 /* register/drop */
#define LG_CMD_SET      0x00000400 /* change properties of static data */
#define LG_CMD_DO       0x00000800 /* change properties of dynamic data */
#define LG_CMD_LOGIN    0x00001000 /* login/logout */
#define LG_CMD_GET      0x00002000 /* query information */
#define LG_CMD_REQUEST	0x00004000 /* requests made by users */
/* other */
#define LG_NETWORK      0x00010000 /* netsplit/netjoin */
#define LG_WALLOPS      0x00020000 /* NOTYET wallops from opers/other servers */
#define LG_RAWDATA      0x00040000 /* all data sent/received */
#define LG_REGISTER     0x00080000 /* all registration related messages */
#define LG_WARN1        0x00100000 /* NOTYET messages formerly walloped */
#define LG_WARN2        0x00200000 /* NOTYET messages formerly snooped */
#define LG_DENYCMD	0x00400000 /* commands denied by security policy */

#define LG_CMD_ALL      0x0000FF00
#define LG_ALL          0x7FFFFFFF /* XXX cannot use bit 31 as it would then be equal to TOKEN_UNMATCHED */

/* aliases for use with logcommand() */
#define CMDLOG_ADMIN    LG_CMD_ADMIN
#define CMDLOG_REGISTER (LG_CMD_REGISTER | LG_REGISTER)
#define CMDLOG_SET      LG_CMD_SET
#define CMDLOG_REQUEST	LG_CMD_REQUEST
#define CMDLOG_DO       LG_CMD_DO
#define CMDLOG_LOGIN    LG_CMD_LOGIN
#define CMDLOG_GET      LG_CMD_GET

E void log_open(void);
E void log_shutdown(void);
E bool log_debug_enabled(void);
E void log_master_set_mask(unsigned int mask);
E logfile_t *logfile_find_mask(unsigned int log_mask);
E void slog(unsigned int level, const char *fmt, ...) PRINTFLIKE(2, 3);
E void logcommand(sourceinfo_t *si, int level, const char *fmt, ...) PRINTFLIKE(3, 4);
E void logcommand_user(service_t *svs, user_t *source, int level, const char *fmt, ...) PRINTFLIKE(4, 5);
E void logcommand_external(service_t *svs, const char *type, connection_t *source, const char *sourcedesc, myuser_t *login, int level, const char *fmt, ...) PRINTFLIKE(7, 8);

/* function.c */

typedef void (*email_canonicalizer_t)(char email[EMAILLEN + 1], void *user_data);

typedef struct {
	email_canonicalizer_t func;
	void *user_data;
	mowgli_node_t node;
} email_canonicalizer_item_t;

/* misc string stuff */
E char *random_string(int sz);
E void create_challenge(sourceinfo_t *si, const char *name, int v, char *dest);
E void tb2sp(char *line);
E char *replace(char *s, int size, const char *old, const char *new);
E const char *number_to_string(int num);
E int validemail(const char *email);
E stringref canonicalize_email(const char *email);
E void canonicalize_email_case(char email[EMAILLEN + 1], void *user_data);
E void register_email_canonicalizer(email_canonicalizer_t func, void *user_data);
E void unregister_email_canonicalizer(email_canonicalizer_t func, void *user_data);
E bool email_within_limits(const char *email);
E bool validhostmask(const char *host);
E char *pretty_mask(char *mask);
E bool validtopic(const char *topic);
E bool has_ctrl_chars(const char *text);
E char *sbytes(float x);
E float bytes(float x);

E unsigned long makekey(void);
E int srename(const char *old_fn, const char *new_fn);

/* time stuff */
#if HAVE_GETTIMEOFDAY
E void s_time(struct timeval *sttime);
E void e_time(struct timeval sttime, struct timeval *ttime);
E int tv2ms(struct timeval *tv);
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
E int sjtoken(char *message, char delimiter, char **parv);
E int tokenize(char *message, char **parv);

/* ubase64.c */
E const char *uinttobase64(char *buf, uint64_t v, int64_t count);
E unsigned int base64touint(const char *buf);
E void decode_p10_ip(const char *b64, char ipstring[HOSTIPLEN]);

/* sharedheap.c */
E mowgli_heap_t *sharedheap_get(size_t size);
E void sharedheap_unref(mowgli_heap_t *heap);
E char *combine_path(const char *parent, const char *child);

#if !HAVE_VSNPRINTF
int rpl_vsnprintf(char *, size_t, const char *, va_list);
#endif
#if !HAVE_SNPRINTF
int rpl_snprintf(char *, size_t, const char *, ...);
#endif
#if !HAVE_VASPRINTF
int rpl_vasprintf(char **, const char *, va_list);
#endif
#if !HAVE_ASPRINTF
int rpl_asprintf(char **, const char *, ...);
#endif

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
