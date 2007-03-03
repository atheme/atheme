/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * XMLRPC memo management functions.
 *
 * $Id: memo.c 7779 2007-03-03 13:55:42Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"xmlrpc/memo", FALSE, _modinit, _moddeinit,
	"$Id: memo.c 7779 2007-03-03 13:55:42Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

/*
 * atheme.memo.send
 *
 * XML inputs:
 *       authcookie, account name, target name, memo text
 *
 * XML outputs:
 *       fault 1  - insufficient parameters
 *       fault 2  - bad parameters
 *       fault 2  - target is sender
 *       fault 2  - memo text too long
 *       fault 3  - sender account does not exist
 *       fault 4  - target account doesn't exist
 *       fault 5  - validation failed
 *       fault 6  - target denies memos
 *       fault 6  - sender is on ignore list of target
 *       fault 9  - memo flood
 *       fault 9  - target inbox full
 *       fault 11  - account not verified
 *       default  - success message
 *
 * Side Effects:
 *       a memo is sent to a user
 */
static int memo_send(void *conn, int parc, char *parv[])
{
	/* Define and initialise structs and variables */
	myuser_t *mu, *tmu = NULL;
	mymemo_t *memo = NULL;
	node_t *n = NULL;
	static char buf[XMLRPC_BUFSIZE] = "";
	char *m = parv[3];

	*buf = '\0';

	if (parc < 4)
	{
		xmlrpc_generic_error(1, "Insufficient parameters.");
		return 0;
	}

	if (!(mu = myuser_find(parv[1])))
	{
		xmlrpc_generic_error(3, "Sending account nonexistent.");
		return 0;
	}

	if (!(tmu = myuser_find(parv[2])))
	{
		xmlrpc_generic_error(4, "Target account nonexistent.");
		return 0;
	}

	if (authcookie_validate(parv[0], mu) == FALSE)
	{
		xmlrpc_generic_error(5, "Authcookie validation failed.");
		return 0;
	}

	if (mu->flags & MU_WAITAUTH)
	{       
		xmlrpc_generic_error(11, "Email address not verified.");
		return 0;
	}

	/* Check whether target is not sender */
	if (tmu == mu)
	{       
		xmlrpc_generic_error(2, "Target is sender.");
		return 0;
	}

	/* Check for user setting "no memos" */
	if (tmu->flags & MU_NOMEMO)
	{       
		xmlrpc_generic_error(6, "Target does not wish to receive memos.");
		return 0;
	}

	/* Check for memo text length */
	if (strlen(m) >= MEMOLEN)
	{       
		xmlrpc_generic_error(2, "Memo text too long.");
		return 0;
	}

	/* Check whether target inbox is full */
	if (tmu->memos.count >= me.mdlimit)
	{       
		xmlrpc_generic_error(9, "Inbox is full.");
		return 0;
	}

	/* Anti-flood */
	if (CURRTIME - mu->memo_ratelimit_time > MEMO_MAX_TIME)
		mu->memo_ratelimit_num = 0;
	if (mu->memo_ratelimit_num > MEMO_MAX_NUM)
	{       
		xmlrpc_generic_error(9, "Memo flood.");
		return 0;
	}		  
	mu->memo_ratelimit_num++; 
	mu->memo_ratelimit_time = CURRTIME;

	/* Check whether sender is being ignored by the target */
	LIST_FOREACH(n, tmu->memo_ignores.head)							
	{
		if (!strcasecmp((char *)n->data, mu->name))
		{
			logcommand_external(memosvs.me, "xmlrpc", conn, NULL, mu, CMDLOG_SET, "failed SEND to %s (on ignore list)", tmu->name);
			xmlrpc_generic_error(6, "Sender is on ignore list.");
			return 0;
		}
	}

	xmlrpc_string(buf, "Memo sent successfully.");
	xmlrpc_send(1, buf);

	logcommand_external(memosvs.me, "xmlrpc", conn, NULL, mu, CMDLOG_SET, "SEND to %s", tmu->name);

	memo = smalloc(sizeof(mymemo_t));
	memo->sent = CURRTIME;
	memo->status = MEMO_NEW;
	strlcpy (memo->sender, mu->name, NICKLEN);
	strlcpy (memo->text, m, MEMOLEN);

	/* Send memo */
	n = node_create();
	node_add(memo, n, &tmu->memos);
	tmu->memoct_new++;

	/* Tell the user that it has a new memo if it is online */
	myuser_notice(memosvs.nick, tmu, "You have a new memo from %s.", mu->name);

	return 0;
}

/*
 * atheme.memo.forward
 *
 * XML inputs:
 *       authcookie, account name, target name, memo id
 *
 * XML outputs:
 *       fault 1  - insufficient parameters
 *       fault 2  - bad parameters
 *       fault 2  - target is sender
 *       fault 3  - sender account does not exist
 *       fault 4  - target account doesn't exist
 *       fault 5  - validation failed
 *       fault 6  - target denies memos
 *       fault 6  - sender is on ignore list of target
 *       fault 9  - memo flood
 *       fault 9  - target inbox full
 *       fault 11 - account not verified
 *       default  - success message
 *
 * Side Effects:
 *       a memo is sent to a user
 */
static int memo_forward(void *conn, int parc, char *parv[])
{
	/* Define and initialise structs and variables */
	user_t *u = user_find_named(parv[1]);
	myuser_t *mu = u->myuser, *tmu = NULL;
	mymemo_t *memo = NULL, *forward = NULL;
	node_t *n = NULL;
	uint8_t i = 1, memonum = atoi(parv[3]);
	static char buf[XMLRPC_BUFSIZE] = "";

	*buf = '\0';

	if (parc < 4)
	{
		xmlrpc_generic_error(1, "Insufficient parameters.");
		return 0;
	}

	if (!(mu = myuser_find(parv[1])))
	{
		xmlrpc_generic_error(3, "Sending account nonexistent.");
		return 0;
	}

	if (!(tmu = myuser_find(parv[2])))
	{
		xmlrpc_generic_error(4, "Target account nonexistent.");
		return 0;
	}

	if (authcookie_validate(parv[0], mu) == FALSE)
	{
		xmlrpc_generic_error(5, "Authcookie validation failed.");
		return 0;
	}

	if (mu->flags & MU_WAITAUTH)
	{       
		xmlrpc_generic_error(11, "Email address not verified.");
		return 0;
	}

	/* Check whether target is not sender */
	if (tmu == mu)
	{       
		xmlrpc_generic_error(2, "Target is sender.");
		return 0;
	}

	/* Check for user setting "no memos" */
	if (tmu->flags & MU_NOMEMO)
	{       
		xmlrpc_generic_error(6, "Target does not wish to receive memos.");
		return 0;
	}

	/* Check whether target inbox is full */
	if (tmu->memos.count >= me.mdlimit)
	{       
		xmlrpc_generic_error(9, "Inbox is full.");
		return 0;
	}

	/* Sanity check on memo ID */
	if (!memonum || memonum > mu->memos.count)
	{
	 	xmlrpc_generic_error(7, "Invalid memo ID.");
		return 0;
	}

	/* Anti-flood */
	if (CURRTIME - mu->memo_ratelimit_time > MEMO_MAX_TIME)
		mu->memo_ratelimit_num = 0;
	if (mu->memo_ratelimit_num > MEMO_MAX_NUM)
	{       
		xmlrpc_generic_error(9, "Memo flood.");
		return 0;
	}		  
	mu->memo_ratelimit_num++; 
	mu->memo_ratelimit_time = CURRTIME;

	/* Check whether sender is being ignored by the target */
	LIST_FOREACH(n, tmu->memo_ignores.head)							
	{
		if (!strcasecmp((char *)n->data, mu->name))
		{
			logcommand_external(memosvs.me, "xmlrpc", conn, NULL, mu, CMDLOG_SET, "failed SEND to %s (on ignore list)", tmu->name);
			xmlrpc_generic_error(6, "Sender is on ignore list.");
			return 0;
		}
	}

	logcommand_external(memosvs.me, "xmlrpc", conn, NULL, mu, CMDLOG_SET, "FORWARD to %s", tmu->name);

	LIST_FOREACH(n, mu->memos.head)
	{
		if (i == memonum)
		{
			/* Allocate memory */
			memo = (mymemo_t *)n->data;
			forward = smalloc(sizeof(mymemo_t));

			/* Duplicate memo */
			memo->sent = CURRTIME;
			memo->status = MEMO_NEW;
			strlcpy (forward->sender, mu->name, NICKLEN);
			strlcpy (forward->text, memo->text, MEMOLEN);

			/* Send memo */
			n = node_create();
			node_add(memo, n, &tmu->memos);
			tmu->memoct_new++;

			/* Tell the user that it has a new memo if it is online */
			myuser_notice(memosvs.nick, tmu, "You have a new memo from %s.", mu->name);

			xmlrpc_string(buf, "Memo sent successfully.");
			xmlrpc_send(1, buf);

			return 0;
		}
	}

	return 0;
}


/*
 * atheme.memo.delete
 *
 * XML inputs:
 *       authcookie, account name, memo id
 *
 * XML outputs:
 *       fault 1 - insufficient parameters
 *       fault 3 - account does not exist
 *       fault 4 - memo id nonexistent
 *       fault 5 - validation failed
 *       default - success message
 *
 * Side Effects:
 *       a memo is deleted
 */
static int memo_delete(void *conn, int parc, char *parv[])
{
	/* Define and initialise structs and variables */
	myuser_t *mu;
	mymemo_t *memo = NULL;
	node_t *n = NULL, *tn = NULL;
	uint8_t i = 0, delcount = 0, memonum = 0, deleteall = 0;
	static char buf[XMLRPC_BUFSIZE] = "";

	*buf = '\0';

	if (parc < 3)
	{
		xmlrpc_generic_error(1, "Insufficient parameters.");
		return 0;
	}

	if (!(mu = myuser_find(parv[1])))
	{
		xmlrpc_generic_error(3, "Account nonexistent.");
		return 0;
	}

	if (authcookie_validate(parv[0], mu) == FALSE)
	{
		xmlrpc_generic_error(5, "Authcookie validation failed.");
		return 0;
	}

	/* Check whether user has memos */
	if (!mu->memos.count)
	{       
        	/* If not, send back an empty string */
		xmlrpc_string(buf, "");
		xmlrpc_send(1, buf);
		return 0;
	}

	/* Enable or don't enable boolean to delete all memos at once */
	if (!strcasecmp("all", parv[2]))
	{       
		deleteall = 1;
	} else {       
		memonum = atoi(parv[2]);
		
		/* Sanity checks */
		if (!memonum || memonum > mu->memos.count)
		{       
			xmlrpc_generic_error(4, "Memo ID invalid.");
			return 0;
		}
	}

	delcount = 0;

	/* Delete memos */
	LIST_FOREACH_SAFE(n, tn, mu->memos.head)
	{       
		i++;
		if ((i == memonum) || (deleteall == 1))
		{       
			delcount++;
			
			memo = (mymemo_t*) n->data;
			
			if (memo->status == MEMO_NEW)
				mu->memoct_new--; /* Decrease memocount */
			
			node_del(n,&mu->memos);
			node_free(n);
			
			free(memo);
		}
	}

	xmlrpc_string(buf, "Memo(s) successfully deleted.");
	xmlrpc_send(1, buf);

	return 0;
}

/*
 * atheme.memo.list
 *
 * XML inputs:
 *       authcookie, account name, list boolean (optional)
 *
 * XML outputs:
 *       fault 1 - insufficient parameters
 *       fault 3 - account does not exist
 *       fault 5 - validation failed
 *       default - success message
 *
 * Side Effects:
 *       none
 */
static int memo_list(void *conn, int parc, char *parv[])
{
	/* Define and initialise structs and variables */
	myuser_t *mu;
	mymemo_t *memo = NULL;
	node_t *n = NULL;
	uint8_t i = 0;
	char timebuf[16] = "", memobuf[64] = "", sendbuf[XMLRPC_BUFSIZE - 1] = "";
	struct tm memotime;
	static char buf[XMLRPC_BUFSIZE] = "";

	*buf = '\0';

	if (parc < 2)
	{
		xmlrpc_generic_error(1, "Insufficient parameters.");
		return 0;
	}

	if (!(mu = myuser_find(parv[1])))
	{
		xmlrpc_generic_error(3, "Account nonexistent.");
		return 0;
	}

	if (authcookie_validate(parv[0], mu) == FALSE)
	{
		xmlrpc_generic_error(5, "Authcookie validation failed.");
		return 0;
	}

	/* Just return a note of memos:unread */
	if (!parv[2]) {
		snprintf(memobuf, 64, "%d:%d", mu->memos.count, mu->memoct_new);
		xmlrpc_string(buf, memobuf);
		xmlrpc_send(1, buf);
		return 0;
	}

	/* Check whether user has memos */
	if (!mu->memos.count)
	{       
        	/* If not, send back an empty string */
		xmlrpc_string(buf, "");
		xmlrpc_send(1, buf);
		return 0;
	}

	LIST_FOREACH(n, mu->memos.head)
	{       
		i++;
		memo = (mymemo_t *)n->data;
		memotime = *localtime(&memo->sent);
		snprintf(timebuf, 16, "%lu", (long unsigned) mktime(&memotime));
		
		if (memo->status == MEMO_NEW)
			snprintf(memobuf, 64, "%d:%s:%s:1\n", i, memo->sender, timebuf);
		else
			snprintf(memobuf, 64, "%d:%s:%s:0\n", i, memo->sender, timebuf);
		strncat(sendbuf, memobuf, 64);
	}

	xmlrpc_string(buf, sendbuf);
	xmlrpc_send(1, buf);

	return 0;
}

/*
 * atheme.memo.read
 *
 * XML inputs:
 *       authcookie, account name, memo id
 *
 * XML outputs:
 *       fault 1 - insufficient parameters
 *       fault 3 - account does not exist
 *       fault 4 - invalid memo id
 *       fault 5 - validation failed
 *       default - success message
 *
 * Side Effects:
 *       marks the memo, if previously marked unread, read
 */
static int memo_read(void *conn, int parc, char *parv[])
{
	/* Define and initialise structs and variables */
	myuser_t *mu;
	mymemo_t *memo = NULL, *receipt = NULL;
	node_t *n = NULL;
	uint32_t i = 1, memonum = 0;
	struct tm memotime;
	char timebuf[16] = "", sendbuf[XMLRPC_BUFSIZE - 1] = "", strfbuf[32] = "";
	static char buf[XMLRPC_BUFSIZE] = "";

	*buf = '\0';

	if (parc < 3)
	{
		xmlrpc_generic_error(1, "Insufficient parameters.");
		return 0;
	}

	if (!(mu = myuser_find(parv[1])))
	{
		xmlrpc_generic_error(4, "Account nonexistent.");
		return 0;
	}

	if (authcookie_validate(parv[0], mu) == FALSE)
	{
		xmlrpc_generic_error(5, "Authcookie validation failed.");
		return 0;
	}

	/* First sanity check */
	if (!parv[2])
	{       
		xmlrpc_generic_error(4, "Invalid memo ID.");
		return 0;
	} else memonum = atoi(parv[2]);

	/* More memo id sanity checks */
	if (!memonum || memonum > mu->memos.count)
	{       
		xmlrpc_generic_error(4, "Invalid memo ID.");
		return 0;
	}

	/* Read memo ids */
	LIST_FOREACH(n, mu->memos.head)
	{       
		if (i == memonum)
		{       
			/* Now, i is the memo id => read the memo */
			memo = (mymemo_t*) n->data;
			memotime = *localtime(&memo->sent);
			snprintf(timebuf, 16, "%lu", (long unsigned) mktime(&memotime));
			strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &memotime);

			/* If the memo is unread, */
			if (memo->status == MEMO_NEW)
			{
				/* mark it as read */
				memo->status = MEMO_READ;
				/* and decrease "new memos" count */
				mu->memoct_new--;
				mu = myuser_find(memo->sender);

				/* If the sender's inbox is not full and is not MemoServ */
				if ((mu != NULL) && (mu->memos.count < me.mdlimit) && strcasecmp(memosvs.nick, memo->sender))
				{       
					receipt = smalloc(sizeof(mymemo_t));
					receipt->sent = CURRTIME;
					receipt->status = MEMO_NEW;
					strlcpy(receipt->sender, memosvs.nick, NICKLEN);
					snprintf(receipt->text, MEMOLEN, "%s has read a memo from you sent at %s", mu->name, strfbuf);

					/* Send a memo to the sender that the memo was read */
					n = node_create();
					node_add(receipt, n, &mu->memos);
					mu->memoct_new++;
				}
			}

			/* Return the memo in the format: id:sender:timestamp:text */
			snprintf(sendbuf, XMLRPC_BUFSIZE - 1, "%d:%s:%s:%s", i, memo->sender, timebuf, memo->text);
			xmlrpc_string(buf, sendbuf);
			xmlrpc_send(1, buf);

			return 0;
		}
		/* Next memo */
		i++;
	}

	return 0;
}

/*
 * atheme.memo.ignore.add
 *
 * XML inputs:
 *       authcookie, account name, target
 *
 * XML outputs:
 *       fault 1 - insufficient parameters
 *       fault 2 - trying to ignore oneself
 *       fault 2 - user already ignored
 *       fault 3 - account does not exist
 *       fault 4 - target does not exist
 *       fault 5 - validation failed
 *       fault 9 - ignore list full
 *       default - success message
 *
 * Side Effects:
 *       ignores user
 */
static int memo_ignore_add(void *conn, int parc, char *parv[])
{
	/* Define and initialise structs and variables */
	myuser_t *mu, *tmu;
	node_t *n, *node;
	char *tmpbuf;
	static char buf[XMLRPC_BUFSIZE] = "";

	*buf = '\0';

	if (parc < 3)
	{
		xmlrpc_generic_error(1, "Insufficient parameters.");
		return 0;
	}

	if (!(mu = myuser_find(parv[1])))
	{
		xmlrpc_generic_error(3, "Account nonexistent.");
		return 0;
	}

	if (authcookie_validate(parv[0], mu) == FALSE)
	{
		xmlrpc_generic_error(5, "Authcookie validation failed.");
		return 0;
	}

	/* Sanity check (target is user) */
	if (!strcasecmp(parv[2], mu->name))
	{       
		xmlrpc_generic_error(2, "You can't ignore yourself.");
		return 0;
	}
	
	/* Check whether target exists */
	if (!(tmu = myuser_find_ext(parv[2])))
	{       
		xmlrpc_generic_error(4, "Target user nonexistent.");
		return 0;
	}
	
	/* Ignore list is full */
	if (mu->memo_ignores.count >= MAXMSIGNORES)
	{       
		xmlrpc_generic_error(9, "Ignore list full.");
		return 0;
	}

	LIST_FOREACH(n, mu->memo_ignores.head)
	{       
		tmpbuf = (char *)n->data;
		
		/* Check whether the user is already being ignored */
		if (!strcasecmp(tmpbuf, parv[2]))
		{       
			xmlrpc_generic_error(2, "That user is already being ignored.");
			return 0;
		}
	}
	
	/* Ignore user */
	tmpbuf = sstrdup(parv[2]);

	node = node_create();
	node_add(tmpbuf, node, &mu->memo_ignores);

	xmlrpc_string(buf, "Operation successful.");
	xmlrpc_send(1, buf);

	return 0;
}

/*
 * atheme.memo.ignore.delete
 *
 * XML inputs:
 *       authcookie, account name, target
 *
 * XML outputs:
 *       fault 1 - insufficient parameters
 *       fault 3 - account does not exist
 *       fault 4 - user not being ignored
 *       fault 5 - validation failed
 *       default - success message
 *
 * Side Effects:
 *       unignores user
 */
static int memo_ignore_delete(void *conn, int parc, char *parv[])
{
	/* Define and initialise structs and variables */
	myuser_t *mu;
	node_t *n, *tn;
	char *tmpbuf;
	static char buf[XMLRPC_BUFSIZE] = "";

	*buf = '\0';

	if (parc < 3)
	{
		xmlrpc_generic_error(1, "Insufficient parameters.");
		return 0;
	}

	if (!(mu = myuser_find(parv[1])))
	{
		xmlrpc_generic_error(3, "Account nonexistent.");
		return 0;
	}

	if (authcookie_validate(parv[0], mu) == FALSE)
	{
		xmlrpc_generic_error(5, "Authcookie validation failed.");
		return 0;
	}

	LIST_FOREACH_SAFE(n, tn, mu->memo_ignores.head)
	{       
		tmpbuf = (char *)n->data;
		
		/* User is in the ignore list */
		if (!strcasecmp(tmpbuf, parv[2]))
		{       
			node_del(n, &mu->memo_ignores);
			node_free(n);

			free(tmpbuf);

			xmlrpc_string(buf, "Operation successful.");
			xmlrpc_send(1, buf);
			return 0;
		}
	}
	
	/* If we reach this spot then the user was not on the ignore list */
	xmlrpc_generic_error(4, "User not on the ignore list.");
	return 0;
}

/*
 * atheme.memo.ignore.clear
 *
 * XML inputs:
 *       authcookie, account name
 *
 * XML outputs:
 *       fault 1 - insufficient parameters
 *       fault 3 - account does not exist
 *       fault 5 - validation failed
 *       default - success message
 *
 * Side Effects:
 *       unignores all users
 */
static int memo_ignore_clear(void *conn, int parc, char *parv[])
{
	/* Define and initialise structs and variables */
	myuser_t *mu;
	node_t *n, *tn;
	static char buf[XMLRPC_BUFSIZE] = "";

	*buf = '\0';

	if (parc < 3)
	{
		xmlrpc_generic_error(1, "Insufficient parameters.");
		return 0;
	}

	if (!(mu = myuser_find(parv[1])))
	{
		xmlrpc_generic_error(3, "Account nonexistent.");
		return 0;
	}

	if (authcookie_validate(parv[0], mu) == FALSE)
	{
		xmlrpc_generic_error(5, "Authcookie validation failed.");
		return 0;
	}

        /* Check whether the user has ignores */
	if (LIST_LENGTH(&mu->memo_ignores) == 0)
	{
        	/* If not, send back an empty string */
		xmlrpc_string(buf, "");
		xmlrpc_send(1, buf);
		return 0;
	}

	LIST_FOREACH_SAFE(n, tn, mu->memo_ignores.head)
	{       
		free(n->data);
		node_del(n, &mu->memo_ignores);
		node_free(n);
	}

	xmlrpc_string(buf, "Operation successful.");
	xmlrpc_send(1, buf);

	return 0;
}

/*
 * atheme.memo.ignore.list
 *
 * XML inputs:
 *       authcookie, account name
 *
 * XML outputs:
 *       fault 1 - insufficient parameters
 *       fault 3 - account does not exist
 *       fault 5 - validation failed
 *       default - success message
 *
 * Side Effects:
 *       none
 */
static int memo_ignore_list(void *conn, int parc, char *parv[])
{
	/* Define and initialise structs and variables */
	myuser_t *mu;
	node_t *n;
	uint8_t i = 1;
	char sendbuf[XMLRPC_BUFSIZE - 1] = "", ignorebuf[64] = "";
	static char buf[XMLRPC_BUFSIZE] = "";

	*buf = '\0';

	if (parc < 3)
	{
		xmlrpc_generic_error(1, "Insufficient parameters.");
		return 0;
	}

	if (!(mu = myuser_find(parv[1])))
	{
		xmlrpc_generic_error(3, "Account nonexistent.");
		return 0;
	}

	if (authcookie_validate(parv[0], mu) == FALSE)
	{
		xmlrpc_generic_error(5, "Authcookie validation failed.");
		return 0;
	}

	LIST_FOREACH(n, mu->memo_ignores.head)
	{
		/* provide a list in the format id:user divided by newlines */
		snprintf(ignorebuf, 64, "%d:%s", i, (char *)n->data);
		strncat(sendbuf, ignorebuf, 64);
		i++;
	}

       	/* If user has no ignores, send back an empty string */
	if (i == 1)
        {
		xmlrpc_string(buf, "");
		xmlrpc_send(1, buf);
		return 0;
        }

	xmlrpc_string(buf, sendbuf);
	xmlrpc_send(1, buf);

	return 0;
}


void _modinit(module_t *m)
{
	xmlrpc_register_method("atheme.memo.send", memo_send);
	xmlrpc_register_method("atheme.memo.forward", memo_forward);
	xmlrpc_register_method("atheme.memo.delete", memo_delete);
	xmlrpc_register_method("atheme.memo.list", memo_list);
	xmlrpc_register_method("atheme.memo.read", memo_read);
	xmlrpc_register_method("atheme.memo.ignore.add", memo_ignore_add);
	xmlrpc_register_method("atheme.memo.ignore.delete", memo_ignore_delete);
	xmlrpc_register_method("atheme.memo.ignore.clear", memo_ignore_clear);
	xmlrpc_register_method("atheme.memo.ignore.list", memo_ignore_list);
}

void _moddeinit(void)
{
	xmlrpc_unregister_method("atheme.memo.send");
	xmlrpc_unregister_method("atheme.memo.forward");
	xmlrpc_unregister_method("atheme.memo.delete");
	xmlrpc_unregister_method("atheme.memo.list");
	xmlrpc_unregister_method("atheme.memo.read");
	xmlrpc_unregister_method("atheme.memo.ignore.add");
	xmlrpc_unregister_method("atheme.memo.ignore.delete");
	xmlrpc_unregister_method("atheme.memo.ignore.clear");
	xmlrpc_unregister_method("atheme.memo.ignore.list");
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
