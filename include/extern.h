/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This header file contains all of the extern's needed.
 *
 * $Id: extern.h 5640 2006-07-02 00:48:37Z jilles $
 */

#ifndef EXTERN_H
#define EXTERN_H

/* save some space/typing */
/* -> moved to atheme.h */

E boolean_t cold_start;

/* cmode.c */
E void channel_mode(user_t *source, channel_t *chan, uint8_t parc, char *parv[]);
E void channel_mode_va(user_t *source, channel_t *chan, uint8_t parc, const char *parv0, ...);
E void clear_simple_modes(channel_t *c);
E char *channel_modes(channel_t *c, boolean_t doparams);
E void modestack_flush_channel(char *channel);
E void modestack_forget_channel(char *channel);
E void modestack_mode_simple(char *source, char *channel, int dir, int32_t flags);
E void modestack_mode_limit(char *source, char *channel, int dir, uint32_t limit);
E void modestack_mode_ext(char *source, char *channel, int dir, int i, const char *value);
E void modestack_mode_param(char *source, char *channel, int dir, char type, const char *value);
E void cmode(char *sender, ...);
E void user_mode(user_t *user, char *modes);
E void check_modes(mychan_t *mychan, boolean_t sendnow);

/* conf.c */
E boolean_t conf_parse(char *);
E void conf_init(void);
E boolean_t conf_rehash(void);
E boolean_t conf_check(void);

/* confp.c */
E void config_free(CONFIGFILE *cfptr);
E CONFIGFILE *config_load(char *filename);
E CONFIGENTRY *config_find(CONFIGENTRY *ceptr, char *name);

/* flags.c */
E void flags_make_bitmasks(const char *string, struct flags_table table[], uint32_t *addflags, uint32_t *removeflags);
E uint32_t flags_to_bitmask(const char *, struct flags_table[], uint32_t flags);
E char *bitmask_to_flags(uint32_t, struct flags_table[]);
E char *bitmask_to_flags2(uint32_t, uint32_t, struct flags_table[]);
E struct flags_table chanacs_flags[];
E uint32_t allow_flags(uint32_t flags);

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
E regex_t *regex_create(char *pattern);
E boolean_t regex_match(regex_t *preg, char *string);
E boolean_t regex_destroy(regex_t *preg);
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

E char *tldprefix;
E boolean_t uses_uid;

/* irc.c */
E void (*parse)(char *line);
E void irc_parse(char *line);
E void p10_parse(char *line);
E struct message_ messages[];
E struct message_ *msg_find(const char *name);

/* match.c */
#define MATCH_RFC1459   0
#define MATCH_ASCII     1

E int match_mapping;

#define IsLower(c)  ((unsigned char)(c) > 0x5f)
#define IsUpper(c)  ((unsigned char)(c) < 0x60)

#define C_ALPHA 0x00000001
#define C_DIGIT 0x00000002

E const unsigned int charattrs[];

#define IsAlpha(c)      (charattrs[(unsigned char) (c)] & C_ALPHA)
#define IsDigit(c)      (charattrs[(unsigned char) (c)] & C_DIGIT)
#define IsAlphaNum(c)   (IsAlpha((c)) || IsDigit((c)))
#define IsNon(c)        (!IsAlphaNum((c)))

E const unsigned char ToLowerTab[];
E const unsigned char ToUpperTab[];

void set_match_mapping(int);

E int ToLower(int);
E int ToUpper(int);

E int irccmp(const char *, const char *);
E int irccasecmp(const char *, const char *);
E int ircncmp(const char *, const char *, int);
E int ircncasecmp(const char *, const char *, int);

E int match(char *, char *);
E char *collapse(char *);

/* node.c */
E list_t soperlist;
E list_t svs_ignore_list;
E list_t tldlist;
E list_t klnlist;
E list_t servlist[HASHSIZE];
E list_t userlist[HASHSIZE];
E list_t chanlist[HASHSIZE];
E list_t mulist[HASHSIZE];
E list_t mclist[HASHSIZE];

E void init_ircpacket(void);
E void init_nodes(void);
/* The following currently only do uplinks -- jilles */
E void mark_all_illegal(void);
E void unmark_all_illegal(void);
E void remove_illegals(void);

E operclass_t *operclass_add(char *name, char *privs);
E void operclass_delete(operclass_t *operclass);
E operclass_t *operclass_find(char *name);

E soper_t *soper_add(char *name, operclass_t *operclass);
E void soper_delete(soper_t *soper);
E soper_t *soper_find(myuser_t *myuser);
E soper_t *soper_find_named(char *name);

E svsignore_t *svsignore_find(user_t *user);
E svsignore_t *svsignore_add(char *mask, char *reason);

E tld_t *tld_add(char *name);
E void tld_delete(char *name);
E tld_t *tld_find(char *name);

E chanban_t *chanban_add(channel_t *chan, const char *mask, int type);
E void chanban_delete(chanban_t *c);
E chanban_t *chanban_find(channel_t *chan, const char *mask, int type);
E void chanban_clear(channel_t *chan);

E server_t *server_add(char *name, uint8_t hops, char *uplink, char *id, char *desc);
E void server_delete(char *name);
E server_t *server_find(char *name);

E user_t *user_add(char *nick, char *user, char *host, char *vhost, char *ip, char *uid, char *gecos, server_t *server, uint32_t ts);
E void user_delete(user_t *u);
E user_t *user_find(const char *nick);
E user_t *user_find_named(const char *nick);
E void user_changeuid(user_t *u, char *uid);

E channel_t *channel_add(char *name, uint32_t ts);
E void channel_delete(char *name);
E channel_t *channel_find(const char *name);

E chanuser_t *chanuser_add(channel_t *chan, char *user);
E void chanuser_delete(channel_t *chan, user_t *user);
E chanuser_t *chanuser_find(channel_t *chan, user_t *user);

E kline_t *kline_add(char *user, char *host, char *reason, long duration);
E void kline_delete(char *user, char *host);
E kline_t *kline_find(char *user, char *host);
E kline_t *kline_find_num(uint32_t number);
E kline_t *kline_find_user(user_t *u);
E void kline_expire(void *arg);

E myuser_t *myuser_add(char *name, char *pass, char *email, uint32_t flags);
E void myuser_delete(myuser_t *mu);
E myuser_t *myuser_find(char *name);
E myuser_t *myuser_find_ext(char *name);
E void myuser_notice(char *from, myuser_t *target, char *fmt, ...);

E mychan_t *mychan_add(char *name);
E void mychan_delete(char *name);
E mychan_t *mychan_find(const char *name);

E chanacs_t *chanacs_add(mychan_t *mychan, myuser_t *myuser, uint32_t level);
E chanacs_t *chanacs_add_host(mychan_t *mychan, char *host, uint32_t level);
E void chanacs_delete(mychan_t *mychan, myuser_t *myuser, uint32_t level);
E void chanacs_delete_host(mychan_t *mychan, char *host, uint32_t level);
E chanacs_t *chanacs_find(mychan_t *mychan, myuser_t *myuser, uint32_t level);
E chanacs_t *chanacs_find_host(mychan_t *mychan, char *host, uint32_t level);
E uint32_t chanacs_host_flags(mychan_t *mychan, char *host);
E chanacs_t *chanacs_find_host_literal(mychan_t *mychan, char *host, uint32_t level);
E chanacs_t *chanacs_find_host_by_user(mychan_t *mychan, user_t *u, uint32_t level);
E uint32_t chanacs_host_flags_by_user(mychan_t *mychan, user_t *u);
E chanacs_t *chanacs_find_by_mask(mychan_t *mychan, char *mask, uint32_t level);
E boolean_t chanacs_user_has_flag(mychan_t *mychan, user_t *u, uint32_t level);
E uint32_t chanacs_user_flags(mychan_t *mychan, user_t *u);
E boolean_t chanacs_change(mychan_t *mychan, myuser_t *mu, char *hostmask, uint32_t *addflags, uint32_t *removeflags, uint32_t restrictflags);
E boolean_t chanacs_change_simple(mychan_t *mychan, myuser_t *mu, char *hostmask, uint32_t addflags, uint32_t removeflags, uint32_t restrictflags);

E void expire_check(void *arg);
/* Check the database for (version) problems common to all backends */
E void db_check(void);

/* services.c */
E int ban(char *sender, char *channel, user_t *user);
E int remove_ban_exceptions(user_t *source, channel_t *chan, user_t *target);
E void join(char *chan, char *nick);
E void initialize_services(void);
E void joinall(char *name);
E void partall(char *name);
E void reintroduce_user(user_t *u);
E void services_init(void);
E void verbose(mychan_t *mychan, char *fmt, ...);
E void snoop(char *fmt, ...);
E void cservice(char *origin, uint8_t parc, char *parv[]);
E void gservice(char *origin, uint8_t parc, char *parv[]);
E void oservice(char *origin, uint8_t parc, char *parv[]);
E void nickserv(char *origin, uint8_t parc, char *parv[]);
E void notice(char *from, char *to, char *msg, ...);

/* atheme.c */
E char *config_file;

/* uid.c */
E void init_uid(void);
E char *uid_get(void);
E void add_one_to_uid(uint32_t i);

/* socket.c */
E int servsock;

E void irc_rhandler(connection_t *cptr);
E int8_t sts(char *fmt, ...);
E int socket_connect(char *host, uint32_t port);
E void reconn(void *arg);
E void io_loop(void);

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

/* protocol stuff */
E void handle_nickchange(user_t *u);
E void handle_burstlogin(user_t *u, char *login);

E void protocol_init(void);

/* signal.c */
E void sighandler(int signum);

/* ptasks.c */
E void handle_version(user_t *);
E void handle_admin(user_t *);
E void handle_info(user_t *);
E void handle_stats(user_t *, char);
E void handle_whois(user_t *, char *);
E void handle_trace(user_t *, char *, char *);
E void handle_motd(user_t *);
E void handle_message(char *, char *, boolean_t, char *);
E void handle_topic(channel_t *, char *, time_t, char *);
E void handle_kill(char *, char *, char *);
E void handle_eob(server_t *);
E int floodcheck(user_t *, user_t *);

/* help.c */
E void help_display(char *svsnick, char *svsdisp, char *origin, char *command, list_t *list);
E void help_addentry(list_t *list, char *topic, char *fname,
	void (*func)(char *origin));
E void help_delentry(list_t *list, char *name);

/* pmodule.c */
E BlockHeap *pcommand_heap;
E BlockHeap *messagetree_heap;

E void verbose_wallops(char *, ...);

#endif /* EXTERN_H */
