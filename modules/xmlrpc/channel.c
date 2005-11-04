/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * XMLRPC channel management functions.
 *
 * $Id: channel.c 3449 2005-11-04 06:47:38Z alambert $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"xmlrpc/channel", FALSE, _modinit, _moddeinit,
	"$Id: channel.c 3449 2005-11-04 06:47:38Z alambert $",
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
static int channel_register(int parc, char *parv[])
{
	myuser_t *mu;
	mychan_t *mc, *tmc;
	node_t *n;
	uint32_t i, tcnt;
	static char buf[XMLRPC_BUFSIZE];

	*buf = '\0';

	if (parc < 2)
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
	mc->mlock_off |= (CMODE_INVITE | CMODE_LIMIT | CMODE_KEY);
	mc->flags |= config_options.defcflags;

	chanacs_add(mc, mu, CA_FOUNDER);

	xmlrpc_string(buf, "Registration successful.");
	xmlrpc_send(1, buf);

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
static int do_metadata_set(int parc, char *parv[])
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

	if (!is_founder(mc, mu) && !is_successor(mc, mu))
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

	xmlrpc_string(buf, "Operation was successful.");
	xmlrpc_send(1, buf);
	return 0;
}

/*
 * atheme.channel.metadata.delete
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
 *       fault 8 - key never existed
 *       default - success message
 *
 * Side Effects:
 *       metadata is added to a channel.
 */ 
static int do_metadata_delete(int parc, char *parv[])
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

	if (!is_founder(mc, mu) && !is_successor(mc, mu))
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
static int do_metadata_get(int parc, char *parv[])
{
	mychan_t *mc;
	metadata_t *md;
	char buf[XMLRPC_BUFSIZE];

	if (parc < 2)
	{
		xmlrpc_generic_error(2, "Insufficient parameters.");
		return 0;
	}

	if (!(mc = mychan_find(parv[2])))
	{
		xmlrpc_generic_error(1, "Unknown channel.");
		return 0;
	}

	/* if private, pretend it doesn't exist */
	if (!(md = metadata_find(mc, METADATA_CHANNEL, parv[1])) || md->private)
	{
		xmlrpc_generic_error(4, "Key does not exist.");
		return 0;
	}

	xmlrpc_string(buf, md->value);
	xmlrpc_send(1, buf);
	return 0;
}

void _modinit(module_t *m)
{
	xmlrpc_register_method("atheme.channel.register", channel_register);
	xmlrpc_register_method("atheme.channel.metadata.set", do_metadata_set);
	xmlrpc_register_method("atheme.channel.metadata.delete", do_metadata_delete);
	xmlrpc_register_method("atheme.channel.metadata.get", do_metadata_get);
}

void _moddeinit(void)
{
	xmlrpc_unregister_method("atheme.channel.register");
	xmlrpc_unregister_method("atheme.channel.metadata.set");
	xmlrpc_unregister_method("atheme.channel.metadata.delete");
	xmlrpc_unregister_method("atheme.channel.metadata.get");
}
