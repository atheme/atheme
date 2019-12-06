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

#include <atheme/stdheaders.h>
#include <atheme/structures.h>

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
#define CMODE_INVITE    0x00000001U
#define CMODE_KEY       0x00000002U
#define CMODE_LIMIT     0x00000004U
#define CMODE_MOD       0x00000008U
#define CMODE_NOEXT     0x00000010U
#define CMODE_PRIV      0x00000040U      /* AKA PARA */
#define CMODE_SEC       0x00000080U
#define CMODE_TOPIC     0x00000100U
#define CMODE_CHANREG	0x00000200U

/* for struct channel -> flags */
#define CHAN_LOG        0x00000001U /* logs sent to here */

/* for struct chanuser -> modes */
#define CSTATUS_OP      0x00000001U
#define CSTATUS_VOICE   0x00000002U
#define CSTATUS_OWNER   0x00000004U      /* unreal/inspircd +q */
#define CSTATUS_PROTECT 0x00000008U      /* unreal/inspircd +a */
#define CSTATUS_HALFOP  0x00000010U      /* unreal/inspircd +h */
#define CSTATUS_IMMUNE	0x00000020U	/* inspircd-style per-user immune */

/* for struct chanban -> flags */
#define CBAN_ANTIFLOOD  0x00000001U	/* chanserv/antiflood set this */

#define MTYPE_NUL 0U
#define MTYPE_ADD 1U
#define MTYPE_DEL 2U

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

/* cmode.c */
char *flags_to_string(unsigned int flags);
int mode_to_flag(char c);
void channel_mode(struct user *source, struct channel *chan, int parc, char *parv[]);
void channel_mode_va(struct user *source, struct channel *chan, int parc, char *parv0, ...);
void clear_simple_modes(struct channel *c);
char *channel_modes(const struct channel *c, bool doparams);
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
