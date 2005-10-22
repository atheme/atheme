/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService TOPIC functions.
 *
 * $Id: topic.c 3079 2005-10-22 07:03:47Z terminal $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/topic", FALSE, _modinit, _moddeinit,
	"$Id: topic.c 3079 2005-10-22 07:03:47Z terminal $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_topic(char *origin);
static void cs_cmd_topicappend(char *origin);
static void cs_fcmd_topic(char *origin, char *chan);
static void cs_fcmd_topicappend(char *origin, char *chan);

command_t cs_topic = { "TOPIC", "Sets a topic on a channel.",
                        AC_NONE, cs_cmd_topic };
command_t cs_topicappend = { "TOPICAPPEND", "Appends a topic on a channel.",
                        AC_NONE, cs_cmd_topicappend };

fcommand_t fc_topic = { "!topic", AC_NONE, cs_fcmd_topic };
fcommand_t fc_topicappend = { "!topicappend", AC_NONE, cs_fcmd_topicappend };

list_t *cs_cmdtree;
list_t *cs_fcmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	cs_cmdtree = module_locate_symbol("chanserv/main", "cs_cmdtree");
	cs_fcmdtree = module_locate_symbol("chanserv/main", "cs_fcmdtree");
	cs_helptree = module_locate_symbol("chanserv/main", "cs_helptree");

        command_add(&cs_topic, cs_cmdtree);
        command_add(&cs_topicappend, cs_cmdtree);
	fcommand_add(&fc_topic, cs_fcmdtree);
	fcommand_add(&fc_topicappend, cs_fcmdtree);

	help_addentry(cs_helptree, "TOPIC", "help/cservice/topic", NULL);
	help_addentry(cs_helptree, "TOPICAPPEND", "help/cservice/topicappend", NULL);
}

void _moddeinit()
{
	command_delete(&cs_topic, cs_cmdtree);
	command_delete(&cs_topicappend, cs_cmdtree);
	fcommand_delete(&fc_topic, cs_fcmdtree);
	fcommand_delete(&fc_topicappend, cs_fcmdtree);

	help_delentry(cs_helptree, "TOPIC");
	help_delentry(cs_helptree, "TOPICAPPEND");
}

static void cs_cmd_topic(char *origin)
{
	char *chan = strtok(NULL, " ");
	char *topic = strtok(NULL, "");
	mychan_t *mc;
	channel_t *c;
	user_t *u;

	if (!chan || !topic)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2TOPIC\2.");
		notice(chansvs.nick, origin, "Syntax: TOPIC <#channel> <topic>");
		return;
	}

	c = channel_find(chan);
	if (!c)
	{
                notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
                return;
        }

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "Channel \2%s\2 is not registered.", chan);
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		notice(chansvs.nick, origin, "\2%s\2 is closed.", chan);
		return;
	}

	u = user_find(origin);

	if (!chanacs_user_has_flag(mc, u, CA_TOPIC))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	if (c->topic)
	{
		free(c->topic);
		free(c->topic_setter);
	}

	c->topic = sstrdup(topic);
	c->topic_setter = sstrdup(origin);

	topic_sts(chan, origin, topic);
	notice(chansvs.nick, origin, "Topic set to \2%s\2 on \2%s\2.", topic, chan);
}

static void cs_cmd_topicappend(char *origin)
{
        char *chan = strtok(NULL, " ");
        char *topic = strtok(NULL, "");
        mychan_t *mc;
        user_t *u;
	char topicbuf[BUFSIZE];
	channel_t *c;

        if (!chan || !topic)
        {
                notice(chansvs.nick, origin, "Insufficient parameters specified for \2TOPICAPPEND\2.");
                notice(chansvs.nick, origin, "Syntax: TOPICAPPEND <#channel> <topic>");
                return;
        }

	c = channel_find(chan);
	if (!c)
	{
                notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
                return;
        }

        mc = mychan_find(chan);
        if (!mc)
        {
                notice(chansvs.nick, origin, "Channel \2%s\2 is not registered.", chan);
                return;
        }

        u = user_find(origin);

        if (!chanacs_user_has_flag(mc, u, CA_TOPIC))
        {
                notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
                return;
        }
        
        if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		notice(chansvs.nick, origin, "\2%s\2 is closed.", chan);
		return;
	}

	topicbuf[0] = '\0';

	if (c->topic)
	{
		strlcpy(topicbuf, c->topic, BUFSIZE);
		strlcat(topicbuf, " | ", BUFSIZE);
		strlcat(topicbuf, topic, BUFSIZE);
		free(c->topic);
		free(c->topic_setter);
		c->topic = sstrdup(topicbuf);
		c->topic_setter = sstrdup(origin);
	}
	else
	{
		strlcpy(topicbuf, topic, BUFSIZE);
		c->topic = sstrdup(topicbuf);
		c->topic_setter = sstrdup(origin);
	}

	topic_sts(chan, origin, topicbuf);
        notice(chansvs.nick, origin, "Topic set to \2%s\2 on \2%s\2.", c->topic, chan);
}


static void cs_fcmd_topic(char *origin, char *chan)
{
        char *topic = strtok(NULL, "");
        mychan_t *mc;
        channel_t *c;
        user_t *u;

        if (!topic)
        {
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2!TOPIC\2.");
		notice(chansvs.nick, origin, "Syntax: !TOPIC <topic>");
                return;
        }

        c = channel_find(chan);
        if (!c)
        {
		/* should not happen */
                return;
        }

        mc = mychan_find(chan);
        if (!mc)
        {
		/* also should not happen */
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
                return;
        }

        u = user_find(origin);

        if (!chanacs_user_has_flag(mc, u, CA_TOPIC))
        {
                notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
                return;
        }
        
        if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		notice(chansvs.nick, origin, "\2%s\2 is closed.", chan);
		return;
	}

        if (c->topic)
        {
                free(c->topic);
                free(c->topic_setter);
        }

        c->topic = sstrdup(topic);
        c->topic_setter = sstrdup(origin);

        topic_sts(chan, origin, topic);
}

static void cs_fcmd_topicappend(char *origin, char *chan)
{
        char *topic = strtok(NULL, "");
        mychan_t *mc;
        user_t *u = user_find(origin);
        char topicbuf[BUFSIZE];
        channel_t *c;

        if (!topic)
        {
                notice(chansvs.nick, origin, "Insufficient parameters specified for \2!TOPICAPPEND\2.");
                notice(chansvs.nick, origin, "Syntax: !TOPICAPPEND <topic>");
                return;
        }

        c = channel_find(chan);
        if (!c)
        {
                notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
                return;
        }

        mc = mychan_find(chan);
        if (!mc)
        {
                notice(chansvs.nick, origin, "Channel \2%s\2 is not registered.", chan);
                return;
        }

        if (!chanacs_user_has_flag(mc, u, CA_TOPIC))
        {
                notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
                return;
        }
        
        if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		notice(chansvs.nick, origin, "\2%s\2 is closed.", chan);
		return;
	}

        topicbuf[0] = '\0';

        if (c->topic)
        {
                strlcpy(topicbuf, c->topic, BUFSIZE);
                strlcat(topicbuf, " | ", BUFSIZE);
                strlcat(topicbuf, topic, BUFSIZE);
		free(c->topic);
		free(c->topic_setter);
                c->topic = sstrdup(topicbuf);
                c->topic_setter = sstrdup(origin);
        }
        else
        {
                strlcpy(topicbuf, topic, BUFSIZE);
                c->topic = sstrdup(topicbuf);
                c->topic_setter = sstrdup(origin);
        }

        topic_sts(chan, origin, topicbuf);
        notice(chansvs.nick, origin, "Topic set to \2%s\2 on \2%s\2.", c->topic, chan);
}
