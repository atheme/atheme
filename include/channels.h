/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 William Pitcock, et al.
 *
 * Data structures for channel information.
 */

#ifndef ATHEME_INC_CHANNELS_H
#define ATHEME_INC_CHANNELS_H 1

#define VALID_GLOBAL_CHANNEL_PFX(name)	(*(name) == '#' || *(name) == '+' || *(name) == '!')
#define VALID_CHANNEL_PFX(name)		(VALID_GLOBAL_CHANNEL_PFX(name) || *(name) == '&')

struct channel
{
	char *          name;
	unsigned int    modes;
	char *          key;
	unsigned int    limit;
	char **         extmodes;       // non-standard simple modes with param eg +j
	unsigned int    nummembers;
	unsigned int    numsvcmembers;
	time_t          ts;
	char *          topic;
	char *          topic_setter;
	time_t          topicts;
	mowgli_list_t   members;
	mowgli_list_t   bans;
	unsigned int    flags;
	struct mychan * mychan;
};

/* struct for channel memberships */
struct chanuser
{
	struct channel *chan;
	struct user *   user;
	unsigned int    modes;
	mowgli_node_t   unode;
	mowgli_node_t   cnode;
};

struct chanban
{
	struct channel *chan;
	char *          mask;
	int             type;   // 'b', 'e', 'I', etc -- jilles
	mowgli_node_t   node;   // for struct channel -> bans
	unsigned int    flags;
};

/* for struct channel -> modes */
#define CMODE_INVITE    0x00000001
#define CMODE_KEY       0x00000002
#define CMODE_LIMIT     0x00000004
#define CMODE_MOD       0x00000008
#define CMODE_NOEXT     0x00000010
#define CMODE_PRIV      0x00000040      /* AKA PARA */
#define CMODE_SEC       0x00000080
#define CMODE_TOPIC     0x00000100
#define CMODE_CHANREG	0x00000200

/* for struct channel -> flags */
#define CHAN_LOG        0x00000001 /* logs sent to here */

/* for struct chanuser -> modes */
#define CSTATUS_OP      0x00000001
#define CSTATUS_VOICE   0x00000002
#define CSTATUS_OWNER   0x00000004      /* unreal/inspircd +q */
#define CSTATUS_PROTECT 0x00000008      /* unreal/inspircd +a */
#define CSTATUS_HALFOP  0x00000010      /* unreal/inspircd +h */
#define CSTATUS_IMMUNE	0x00000020	/* inspircd-style per-user immune */

/* for struct chanban -> flags */
#define CBAN_ANTIFLOOD  0x00000001	/* chanserv/antiflood set this */

#define MTYPE_NUL 0
#define MTYPE_ADD 1
#define MTYPE_DEL 2

struct cmode
{
	char            mode;
	unsigned int    value;
};

struct extmode
{
	char    mode;
	bool  (*check)(const char *, struct channel *, struct mychan *, struct user *, struct myuser *);
};

/* channel related hooks */
typedef struct {

	/* Write NULL here if you kicked the user. When kicking the last user, you must join a service first,
	 * otherwise the channel may be destroyed and crashes may occur. The service may not part until you
	 * return; chanserv provides MC_INHABIT to help with this. This also prevents kick/rejoin floods. If
	 * this is NULL, a previous function kicked the user
	 */
	struct chanuser *       cu;

} hook_channel_joinpart_t;

typedef struct {
	struct user *   u;
	struct channel *c;
	char *          msg;
} hook_cmessage_data_t;

typedef struct {
	struct user *   u;              // Online user that changed the topic
	struct server * s;              // Server that restored a topic
	struct channel *c;              // Channel still has old topic
	const char *    setter;         // Stored setter string, can be nick, nick!user@host or server
	time_t          ts;             // Time the topic was changed
	const char *    topic;          // New topic
	int             approved;       // Write non-zero here to cancel the change
} hook_channel_topic_check_t;

typedef struct {
	struct user *   u;
	struct channel *c;
} hook_channel_mode_t;

typedef struct {
	struct chanuser *       cu;
	const char              mchar;
	const unsigned int      mvalue;
} hook_channel_mode_change_t;

/* cmode.c */
char *flags_to_string(unsigned int flags);
int mode_to_flag(char c);
void channel_mode(struct user *source, struct channel *chan, int parc, char *parv[]);
void channel_mode_va(struct user *source, struct channel *chan, int parc, char *parv0, ...);
void clear_simple_modes(struct channel *c);
char *channel_modes(struct channel *c, bool doparams);
void modestack_flush_channel(struct channel *channel);
void modestack_forget_channel(struct channel *channel);
void modestack_finalize_channel(struct channel *channel);
void check_modes(struct mychan *mychan, bool sendnow);

void modestack_mode_simple_real(const char *source, struct channel *channel, int dir, int flags);
void modestack_mode_limit_real(const char *source, struct channel *channel, int dir, unsigned int limit);
void modestack_mode_ext_real(const char *source, struct channel *channel, int dir, unsigned int i, const char *value);
void modestack_mode_param_real(const char *source, struct channel *channel, int dir, char type, const char *value);

extern void (*modestack_mode_simple)(const char *source, struct channel *channel, int dir, int flags);
extern void (*modestack_mode_limit)(const char *source, struct channel *channel, int dir, unsigned int limit);
extern void (*modestack_mode_ext)(const char *source, struct channel *channel, int dir, unsigned int i, const char *value);
extern void (*modestack_mode_param)(const char *source, struct channel *channel, int dir, char type, const char *value);

void modestack_flush_now(void);

/* channels.c */
extern mowgli_patricia_t *chanlist;

void init_channels(void);

struct channel *channel_add(const char *name, time_t ts, struct server *creator);
void channel_delete(struct channel *c);
//inline struct channel *channel_find(const char *name);

struct chanuser *chanuser_add(struct channel *chan, const char *user);
void chanuser_delete(struct channel *chan, struct user *user);
struct chanuser *chanuser_find(struct channel *chan, struct user *user);

struct chanban *chanban_add(struct channel *chan, const char *mask, int type);
void chanban_delete(struct chanban *c);
struct chanban *chanban_find(struct channel *chan, const char *mask, int type);
//inline void chanban_clear(struct channel *chan);

#endif /* !ATHEME_INC_CHANNELS_H */
