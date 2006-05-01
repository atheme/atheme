/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * XMLRPC channel management functions.
 *
 * $Id: channel.c 5161 2006-05-01 17:09:39Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"xmlrpc/channel", FALSE, _modinit, _moddeinit,
	"$Id: channel.c 5161 2006-05-01 17:09:39Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

/*
 * atheme.channel.register
 *
 * XML inputs:
 *       authcookie, account name, channel name
 *
 * XML outputs:
 *       fault 1 - validation failed
 *       fault 2 - unknown account
 *       fault 3 - bad parameters
 *       fault 4 - insufficient parameters
 *       fault 5 - channel is already registered
 *       fault 6 - channel is on IRC (would be unfair to claim ownership)
 *       fault 7 - too many channels registered to this account
 *       default - success message
 *
 * Side Effects:
 *       a channel is registered in the system
 */
static int channel_register(void *conn, int parc, char *parv[])
{
	myuser_t *mu;
	mychan_t *mc, *tmc;
	node_t *n;
	uint32_t i, tcnt;
	static char buf[XMLRPC_BUFSIZE];

	*buf = '\0';

	if (parc < 3)
	{
		xmlrpc_generic_error(4, "Insufficient parameters.");
		return 0;
	}

	if (!(mu = myuser_find(parv[1])))
	{
		xmlrpc_generic_error(2, "Unknown account.");
		return 0;
	}

	if (authcookie_validate(parv[0], mu) == FALSE)
	{
		xmlrpc_generic_error(1, "Authcookie validation failed.");
		return 0;
	}

	if ((parv[2][0] != '#') || strchr(parv[2], ',') || strchr(parv[2], ' ')
		|| strchr(parv[2], '\r') || strchr(parv[2], '\n'))
	{
		xmlrpc_generic_error(3, "The channel name is invalid.");
		return 0;
	}

	if (channel_find(parv[2]))
	{
		xmlrpc_generic_error(6, "A channel matching this name is already on IRC.");
		return 0;
	}

	if (mychan_find(parv[2]))
	{
		xmlrpc_generic_error(5, "The channel is already registered.");
		return 0;
	}

	for (i = 0, tcnt = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mclist[i].head)
		{
			tmc = (mychan_t *)n->data;

			if (is_founder(tmc, mu))
				tcnt++;
		}
	}

	if (tcnt >= me.maxchans)
	{
		xmlrpc_generic_error(7, "Too many channels are associated with this account.");
		return 0;
	}

	snoop("REGISTER: \2%s\2 to \2%s\2 (via \2xmlrpc\2)", parv[2], parv[1]);

	mc = mychan_add(parv[2]);
	mc->founder = mu;
	mc->registered = CURRTIME;
	mc->used = CURRTIME;
	mc->mlock_on |= (CMODE_NOEXT | CMODE_TOPIC);
	mc->mlock_off |= (CMODE_LIMIT | CMODE_KEY);
	mc->flags |= config_options.defcflags;

	chanacs_add(mc, mu, CA_INITIAL);

	xmlrpc_string(buf, "Registration successful.");
	xmlrpc_send(1, buf);

	logcommand_external(chansvs.me, "xmlrpc", conn, mu, CMDLOG_REGISTER, "%s REGISTER", mc->name);

	hook_call_event("channel_register", mc);

	return 0;
}

/*
 * atheme.channel.metadata.set
 *
 * XML inputs:
 *       authcookie, account name, channel name, key, value
 *
 * XML outputs:
 *       fault 1 - validation failed
 *       fault 2 - unknown account
 *       fault 4 - insufficient parameters
 *       fault 5 - unknown channel
 *       fault 6 - no access
 *       fault 7 - bad parameters
 *       fault 8 - table full
 *       default - success message
 *
 * Side Effects:
 *       metadata is added to a channel.
 */ 
static int do_metadata_set(void *conn, int parc, char *parv[])
{
	myuser_t *mu;
	mychan_t *mc;
	char buf[XMLRPC_BUFSIZE];

	if (parc < 5)
	{
		xmlrpc_generic_error(4, "Insufficient parameters.");
		return 0;
	}

	if (!(mu = myuser_find(parv[1])))
	{
		xmlrpc_generic_error(2, "Unknown account.");
		return 0;
	}

	if (authcookie_validate(parv[0], mu) == FALSE)
	{
		xmlrpc_generic_error(1, "Authcookie validation failed.");
		return 0;
	}

	if (!(mc = mychan_find(parv[2])))
	{
		xmlrpc_generic_error(5, "Unknown channel.");
		return 0;
	}

	if (!is_founder(mc, mu))
	{
		xmlrpc_generic_error(6, "No access.");
		return 0;
	}

	if (strchr(parv[3], ':')
		|| (strlen(parv[3]) > 32) || (strlen(parv[4]) > 300)
		|| strchr(parv[3], '\r') || strchr(parv[3], '\n') || strchr(parv[3], ' ')
		|| strchr(parv[4], '\r') || strchr(parv[4], '\n') || strchr(parv[4], ' '))
	{
		xmlrpc_generic_error(7, "Bad parameters.");
		return 0;
	}

	if (mc->metadata.count >= me.mdlimit)
	{
		xmlrpc_generic_error(8, "Metadata table full.");
		return 0;
	}

	metadata_add(mc, METADATA_CHANNEL, parv[3], parv[4]);

	logcommand_external(chansvs.me, "xmlrpc", conn, mu, CMDLOG_SET, "%s SET PROPERTY %s to %s", mc->name, parv[3], parv[4]);

	xmlrpc_string(buf, "Operation was successful.");
	xmlrpc_send(1, buf);
	return 0;
}

/*
 * atheme.channel.metadata.delete
 *
 * XML inputs:
 *       authcookie, account name, channel name, key
 *
 * XML outputs:
 *       fault 1 - validation failed
 *       fault 2 - unknown account
 *       fault 4 - insufficient parameters
 *       fault 5 - unknown channel
 *       fault 6 - no access
 *       fault 7 - bad parameters
 *       fault 8 - key never existed
 *       default - success message
 *
 * Side Effects:
 *       metadata is added to a channel.
 */ 
static int do_metadata_delete(void *conn, int parc, char *parv[])
{
	myuser_t *mu;
	mychan_t *mc;
	char buf[XMLRPC_BUFSIZE];

	if (parc < 4)
	{
		xmlrpc_generic_error(4, "Insufficient parameters.");
		return 0;
	}

	if (!(mu = myuser_find(parv[1])))
	{
		xmlrpc_generic_error(2, "Unknown account.");
		return 0;
	}

	if (authcookie_validate(parv[0], mu) == FALSE)
	{
		xmlrpc_generic_error(1, "Authcookie validation failed.");
		return 0;
	}

	if (!(mc = mychan_find(parv[2])))
	{
		xmlrpc_generic_error(5, "Unknown channel.");
		return 0;
	}

	if (!is_founder(mc, mu))
	{
		xmlrpc_generic_error(6, "No access.");
		return 0;
	}

	if (strchr(parv[3], ':') || (strlen(parv[3]) > 32)
		|| strchr(parv[3], '\r') || strchr(parv[3], '\n') || strchr(parv[3], ' '))
	{
		xmlrpc_generic_error(7, "Bad parameters.");
		return 0;
	}

	if (!metadata_find(mc, METADATA_CHANNEL, parv[3]))
	{
		xmlrpc_generic_error(8, "Key does not exist.");
		return 0;
	}

	metadata_delete(mc, METADATA_CHANNEL, parv[3]);

	logcommand_external(chansvs.me, "xmlrpc", conn, mu, CMDLOG_SET, "%s SET PROPERTY %s (deleted)", mc->name, parv[3]);

	xmlrpc_string(buf, "Operation was successful.");
	xmlrpc_send(1, buf);
	return 0;
}

/*
 * atheme.channel.metadata.get
 *
 * XML inputs:
 *       channel name, key
 *
 * XML outputs:
 *       fault 1 - unknown channel
 *       fault 2 - insufficient parameters
 *       fault 3 - invalid parameters
 *       fault 4 - key doesn't exist
 *       default - value
 *
 * Side Effects:
 *       none.
 */ 
static int do_metadata_get(void *conn, int parc, char *parv[])
{
	mychan_t *mc;
	metadata_t *md;
	char buf[XMLRPC_BUFSIZE];

	if (parc < 2)
	{
		xmlrpc_generic_error(2, "Insufficient parameters.");
		return 0;
	}

	if (!(mc = mychan_find(parv[0])))
	{
		xmlrpc_generic_error(1, "Unknown channel.");
		return 0;
	}

	if ((strlen(parv[1]) > 32)
		|| strchr(parv[1], '\r') || strchr(parv[1], '\n') || strchr(parv[1], ' '))
	{
		xmlrpc_generic_error(3, "Invalid parameters.");
		return 0;
	}

	/* if private, pretend it doesn't exist */
	if (!(md = metadata_find(mc, METADATA_CHANNEL, parv[1])) || md->private)
	{
		xmlrpc_generic_error(4, "Key does not exist.");
		return 0;
	}

	logcommand_external(chansvs.me, "xmlrpc", conn, NULL, CMDLOG_GET, "%s GET PROPERTY %s", mc->name, parv[3]);

	xmlrpc_string(buf, md->value);
	xmlrpc_send(1, buf);
	return 0;
}

/*
 * atheme.channel.topic.set
 *
 * XML inputs:
 *       authcookie, account name, channel name, topic
 *
 * XML outputs:
 *       fault 1 - validation failed
 *       fault 2 - unknown account
 *       fault 4 - insufficient parameters
 *       fault 5 - unknown channel
 *       fault 6 - no access
 *       fault 7 - bad parameters
 *       default - success message
 *
 * Side Effects:
 *       a channel's topic is set.
 */ 
static int do_topic_set(void *conn, int parc, char *parv[])
{
	char *chan = strtok(NULL, " ");
	myuser_t *mu;
	mychan_t *mc;
	channel_t *c;
	char buf[XMLRPC_BUFSIZE];
 
	if (parc < 4)
	{
		xmlrpc_generic_error(4, "Insufficient parameters.");
		return 0;
	}
 
	if (!(mu = myuser_find(parv[1])))
	{
		xmlrpc_generic_error(2, "Unknown account.");
		return 0;
	}
 
	if (authcookie_validate(parv[0], mu) == FALSE)
	{
		xmlrpc_generic_error(1, "Authcookie validation failed.");
		return 0;
	}
 
	if (!(mc = mychan_find(parv[2])))
	{
		xmlrpc_generic_error(5, "Unknown channel.");
		return 0;
	}

	if (!chanacs_find(mc, mu, CA_TOPIC))
	{
		xmlrpc_generic_error(6, "No access.");
		return 0;
	}
 
	if (strlen(parv[3]) > 300 || strchr(parv[3], '\r') || strchr(parv[3], '\n'))
	{
		xmlrpc_generic_error(7, "Bad parameters.");
		return 0;
	}

	if(!(c = channel_find(parv[2])))
		return 0;

	handle_topic(c, parv[1], CURRTIME, parv[3]);
	topic_sts(parv[2], parv[1], CURRTIME, parv[3]);
 
	logcommand_external(chansvs.me, "xmlrpc", conn, mu, CMDLOG_SET, "%s TOPIC %s", mc->name, parv[2]);
 
	xmlrpc_string(buf, "Topic Changed.");
	xmlrpc_send(1, buf);
	return 0;
}

/*
 * atheme.channel.topic.append
 *
 * XML inputs:
 *       authcookie, account name, channel name, topic
 *
 * XML outputs:
 *       fault 1 - validation failed
 *       fault 2 - unknown account
 *       fault 4 - insufficient parameters
 *       fault 5 - unknown channel
 *       fault 6 - no access
 *       fault 7 - bad parameters
 *       default - success message
 *
 * Side Effects:
 *       a channel's topic is appended.
 */ 
static int do_topic_append(void *conn, int parc, char *parv[])
{
	char *chan = strtok(NULL, " ");
	myuser_t *mu;
	mychan_t *mc;
	channel_t *c;
	char buf[XMLRPC_BUFSIZE], topicbuf[BUFSIZE];
 
	if (parc < 4)
	{
		xmlrpc_generic_error(4, "Insufficient parameters.");
		return 0;
	}
 
	if (!(mu = myuser_find(parv[1])))
	{
		xmlrpc_generic_error(2, "Unknown account.");
		return 0;
	}
 
	if (authcookie_validate(parv[0], mu) == FALSE)
	{
		xmlrpc_generic_error(1, "Authcookie validation failed.");
		return 0;
	}
 
	if (!(mc = mychan_find(parv[2])))
	{
		xmlrpc_generic_error(5, "Unknown channel.");
		return 0;
	}

	if (!chanacs_find(mc, mu, CA_TOPIC))
	{
		xmlrpc_generic_error(6, "No access.");
		return 0;
	}
 
	if(!(c = channel_find(parv[2])))
		return 0;

	if (c->topic)
	{
		strlcpy(topicbuf, c->topic, BUFSIZE);
		strlcat(topicbuf, " | ", BUFSIZE);
		strlcat(topicbuf, parv[3], BUFSIZE);
	}
	else
		strlcpy(topicbuf, parv[3], BUFSIZE);

	if (strlen(topicbuf) > 300 || strchr(topicbuf, '\r') || strchr(topicbuf, '\n'))
	{
		xmlrpc_generic_error(7, "Bad parameters.");
		return 0;
	}

	handle_topic(c, parv[1], CURRTIME, topicbuf);
	topic_sts(parv[2], parv[1], CURRTIME, topicbuf);
 
	logcommand_external(chansvs.me, "xmlrpc", conn, mu, CMDLOG_SET, "%s TOPICAPPEND %s", mc->name, parv[2]);
 
	xmlrpc_string(buf, "Topic Changed.");
	xmlrpc_send(1, buf);
	return 0;
}

/*
 * atheme.channel.access.get
 *
 * XML inputs:
 *       authcookie, account name, channel name
 *
 * XML outputs:
 *       fault 1 - validation failed
 *       fault 2 - unknown account
 *       fault 4 - insufficient parameters
 *       fault 5 - unknown channel
 *       default - value (string starting with '+')
 *
 * Side Effects:
 *       none.
 */ 
static int do_access_get(void *conn, int parc, char *parv[])
{
	myuser_t *mu, *target;
	mychan_t *mc;
	chanacs_t *ca;
	char buf[XMLRPC_BUFSIZE];

	if (parc < 3)
	{
		xmlrpc_generic_error(4, "Insufficient parameters.");
		return 0;
	}

	if (!(mu = myuser_find(parv[1])))
	{
		xmlrpc_generic_error(2, "Unknown account.");
		return 0;
	}

	if (authcookie_validate(parv[0], mu) == FALSE)
	{
		xmlrpc_generic_error(1, "Authcookie validation failed.");
		return 0;
	}
 
	if (!(mc = mychan_find(parv[2])))
	{
		xmlrpc_generic_error(5, "Unknown channel.");
		return 0;
	}

	logcommand_external(chansvs.me, "xmlrpc", conn, NULL, CMDLOG_GET, "%s GET ACCESS", mc->name);

	ca = chanacs_find(mc, mu, 0);

	xmlrpc_string(buf, bitmask_to_flags(ca != NULL ? ca->level : 0, chanacs_flags));
	xmlrpc_send(1, buf);
	return 0;
}


void _modinit(module_t *m)
{
	xmlrpc_register_method("atheme.channel.register", channel_register);
	xmlrpc_register_method("atheme.channel.metadata.set", do_metadata_set);
	xmlrpc_register_method("atheme.channel.metadata.delete", do_metadata_delete);
	xmlrpc_register_method("atheme.channel.metadata.get", do_metadata_get);
	xmlrpc_register_method("atheme.channel.topic.set", do_topic_set);
	xmlrpc_register_method("atheme.channel.topic.append", do_topic_append);
	xmlrpc_register_method("atheme.channel.access.get", do_access_get);
}

void _moddeinit(void)
{
	xmlrpc_unregister_method("atheme.channel.register");
	xmlrpc_unregister_method("atheme.channel.metadata.set");
	xmlrpc_unregister_method("atheme.channel.metadata.delete");
	xmlrpc_unregister_method("atheme.channel.metadata.get");
	xmlrpc_unregister_method("atheme.channel.topic.set");
	xmlrpc_unregister_method("atheme.channel.topic.append");
	xmlrpc_unregister_method("atheme.channel.access.get");
}
