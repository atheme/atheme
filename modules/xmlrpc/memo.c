#include "atheme.h"

DECLARE_MODULE_V1
(
	"xmlrpc/memo", FALSE, _modinit, _moddeinit,
	"$Id: memo.c revno date pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

/*
 * atheme.memo.send
 *
 * XML inputs:
 *       authcookie, account name, target name, memo text
 *
 * XML outputs:
 *       fault 1 - validation failed
 *       fault 2 - sender account does not exist
 *       fault 3 - bad parameters
 *       fault 4 - insufficient parameters
 *       fault 5 - target account doesn't exist
 *       fault 6 - account not verified
 *       fault 7 - target is sender
 *       fault 8 - target denies memos
 *       fault 9 - memo text too long
 *       fault 10 - sender is on ignore list of target
 *       fault 11 - memo flood
 *       fault 12 - target inbox full
 *       default - success message
 *
 * Side Effects:
 *       a memo is sent to a user
 */
static int memo_send(void *conn, int parc, char *parv[])
{
	user_t *sender = user_find_named(parv[1]), *target = NULL;
	myuser_t *mysender = sender->myuser, *mytarget = NULL;
	mymemo_t *memo = NULL;
	node_t *n = NULL;
	static char buf[XMLRPC_BUFSIZE] = "";
	char *m = parv[3];

	*buf = '\0';

	if (parc < 4)
	{
		xmlrpc_generic_error(4, "Insufficient parameters.");
		return 0;
	}

	if (!(mysender = myuser_find(parv[1])))
	{
		xmlrpc_generic_error(2, "Sending account nonexistent.");
		return 0;
	}

	if (!(mytarget = myuser_find(parv[2])))
	{
		xmlrpc_generic_error(5, "Target account nonexistent.");
		return 0;
	}

	if (authcookie_validate(parv[0], mysender) == FALSE)
	{
		xmlrpc_generic_error(1, "Authcookie validation failed.");
		return 0;
	}

	if (mysender->flags & MU_WAITAUTH)
	{       
		xmlrpc_generic_error(6, "Email address not verified.");
		return 0;
	}

	/* Check whether target is not sender */
	if (mytarget == mysender)
	{       
		xmlrpc_generic_error(7, "Target is sender.");
		return 0;
	}

	/* Check for user setting "no memos" */
	if (mytarget->flags & MU_NOMEMO)
	{       
		xmlrpc_generic_error(8, "Target does not wish to receive memos.");
		return 0;
	}

	/* Check for memo text length */
	if (strlen(m) >= MEMOLEN)
	{       
		xmlrpc_generic_error(9, "Memo text too long.");
		return 0;
	}

	/* Check whether target inbox is full */
	if (mytarget->memos.count >= me.mdlimit)
	{       
		xmlrpc_generic_error(12, "Inbox is full.");
		return 0;
	}

	/* Anti-flood */
	if (CURRTIME - mysender->memo_ratelimit_time > MEMO_MAX_TIME)
		mysender->memo_ratelimit_num = 0;
	if (mysender->memo_ratelimit_num > MEMO_MAX_NUM)
	{       
		xmlrpc_generic_error(11, "Memo flood.");
		return 0;
	}		  
	mysender->memo_ratelimit_num++; 
	mysender->memo_ratelimit_time = CURRTIME;

	/* Check whether sender is being ignored by the target */
	LIST_FOREACH(n, mytarget->memo_ignores.head)							
	{
		if (!strcasecmp((char *)n->data, mysender->name))
		{
			logcommand_external(memosvs.me, "xmlrpc", conn, mysender, CMDLOG_SET, "failed SEND to %s (on ignore list)", mytarget->name);
			xmlrpc_generic_error(10, "Sender is on ignore list.");
			return 0;
		}
	}

	xmlrpc_string(buf, "Memo sent successfully.");
	xmlrpc_send(1, buf);

	logcommand_external(memosvs.me, "xmlrpc", conn, mysender, CMDLOG_SET, "SEND to %s", mytarget->name);

	memo = smalloc(sizeof(mymemo_t));
	memo->sent = CURRTIME;
	memo->status = MEMO_NEW;
	strlcpy (memo->sender, mysender->name, NICKLEN);
	strlcpy (memo->text, m, MEMOLEN);

	/* Send memo */
	n = node_create();
	node_add(memo, n, &mytarget->memos);
	mytarget->memoct_new++;

	/* Tell the user that it has a new memo if it is online */
	target = user_find_named(parv[2]);
	if (!irccmp(parv[1], mysender->name))
		myuser_notice(memosvs.nick, mytarget, "You have a new memo from %s.", mysender->name);
	else    
		myuser_notice(memosvs.nick, mytarget, "You have a new memo from %s (nick: %s).", mysender->name, parv[1]);

	return 0;
}

/*
 * atheme.memo.delete
 *
 * XML inputs:
 *       authcookie, account name, memo id
 *
 * XML outputs:
 *       fault 1 - validation failed
 *       fault 2 - account does not exist
 *       fault 3 - memo id nonexistent
 *       fault 4 - insufficient parameters
 *       fault 5 - no memos
 *       default - success message
 *
 * Side Effects:
 *       a memo is deleted
 */
static int memo_delete(void *conn, int parc, char *parv[])
{
	user_t *user = user_find_named(parv[1]);
	myuser_t *myuser = user->myuser;
	mymemo_t *memo = NULL;
	node_t *n = NULL, *tn = NULL;
	uint8_t i = 0, delcount = 0, memonum = 0, deleteall = 0;
	static char buf[XMLRPC_BUFSIZE] = "";

	*buf = '\0';

	if (parc < 3)
	{
		xmlrpc_generic_error(4, "Insufficient parameters.");
		return 0;
	}

	if (!(myuser = myuser_find(parv[1])))
	{
		xmlrpc_generic_error(2, "Account nonexistent.");
		return 0;
	}

	if (authcookie_validate(parv[0], myuser) == FALSE)
	{
		xmlrpc_generic_error(1, "Authcookie validation failed.");
		return 0;
	}

	/* Check whether user has memos */
	if (!myuser->memos.count)
	{       
		xmlrpc_generic_error(5, "No memos to delete.");
		return 0;
	}

	/* Enable or don't enable boolean to delete all memos at once */
	if (!strcasecmp("all", parv[2]))
	{       
		deleteall = 1;
	} else {       
		memonum = atoi(parv[2]);
		
		/* Sanity checks */
		if (!memonum || memonum > myuser->memos.count)
		{       
			xmlrpc_generic_error(3, "Memo ID invalid.");
			return 0;
		}
	}

	delcount = 0;

	/* Delete memos */
	LIST_FOREACH_SAFE(n, tn, myuser->memos.head)
	{       
		i++;
		if ((i == memonum) || (deleteall == 1))
		{       
			delcount++;
			
			memo = (mymemo_t*) n->data;
			
			if (memo->status == MEMO_NEW)
				myuser->memoct_new--; /* Decrease memocount */
			
			node_del(n,&myuser->memos);
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
 *       fault 1 - validation failed
 *       fault 2 - account does not exist
 *       fault 3 - insufficient parameters
 *       fault 4 - no memos
 *       default - success message
 *
 * Side Effects:
 *       none
 */
static int memo_list(void *conn, int parc, char *parv[])
{
	user_t *user = user_find_named(parv[1]);
	myuser_t *myuser = user->myuser;
	mymemo_t *memo = NULL;
	node_t *n = NULL;
	uint8_t i = 0;
	char timebuf[16] = "", memobuf[64] = "", sendbuf[XMLRPC_BUFSIZE - 1] = "";
	struct tm memotime;
	static char buf[XMLRPC_BUFSIZE] = "";

	*buf = '\0';

	if (parc < 2)
	{
		xmlrpc_generic_error(3, "Insufficient parameters.");
		return 0;
	}

	if (!(myuser = myuser_find(parv[1])))
	{
		xmlrpc_generic_error(2, "Account nonexistent.");
		return 0;
	}

	if (authcookie_validate(parv[0], myuser) == FALSE)
	{
		xmlrpc_generic_error(1, "Authcookie validation failed.");
		return 0;
	}

	/* Just return a note of memos:unread */
	if (!parv[2]) {
		snprintf(memobuf, 64, "%d:%d", myuser->memos.count, myuser->memoct_new);
		xmlrpc_string(buf, memobuf);
		xmlrpc_send(1, buf);
		return 0;
	}

	/* Check whether user has memos */
	if (!myuser->memos.count)
	{       
		xmlrpc_generic_error(4, "No memos.");
		return 0;
	}

	LIST_FOREACH(n, myuser->memos.head)
	{       
		i++;
		memo = (mymemo_t *)n->data;
		memotime = *localtime(&memo->sent);
		snprintf(timebuf, 16, "%d", mktime(&memotime));
		
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
 *       fault 1 - validation failed
 *       fault 2 - account does not exist
 *       fault 3 - insufficient parameters
 *       fault 4 - invalid memo id
 *       default - success message
 *
 * Side Effects:
 *       marks the memo, if previously marked unread, read
 */
static int memo_read(void *conn, int parc, char *parv[])
{
	user_t *user = user_find_named(parv[1]), *sender = NULL;
	myuser_t *myuser = user->myuser, *mysender = NULL;
	mymemo_t *memo = NULL, *receipt = NULL;
	node_t *n = NULL;
	uint32_t i = 1, memonum = 0;
	struct tm memotime;
	char timebuf[16] = "", sendbuf[XMLRPC_BUFSIZE - 1] = "", strfbuf[32] = "";
	static char buf[XMLRPC_BUFSIZE] = "";

	*buf = '\0';

	if (parc < 3)
	{
		xmlrpc_generic_error(3, "Insufficient parameters.");
		return 0;
	}

	if (!(myuser = myuser_find(parv[1])))
	{
		xmlrpc_generic_error(2, "Account nonexistent.");
		return 0;
	}

	if (authcookie_validate(parv[0], myuser) == FALSE)
	{
		xmlrpc_generic_error(1, "Authcookie validation failed.");
		return 0;
	}

	/* First sanity check */
	if (!parv[2])
	{       
		xmlrpc_generic_error(4, "Invalid memo ID.");
		return 0;
	} else memonum = atoi(parv[2]);

	/* More memo id sanity checks */
	if (!memonum || memonum > myuser->memos.count)
	{       
		xmlrpc_generic_error(4, "Invalid memo ID.");
		return 0;
	}

	/* Read memo ids */
	LIST_FOREACH(n, myuser->memos.head)
	{       
		if (i == memonum)
		{       
			/* Now, i is the memo id => read the memo */
			memo = (mymemo_t*) n->data;
			memotime = *localtime(&memo->sent);
			snprintf(timebuf, 16, "%d", mktime(&memotime));
			strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &memotime);

			/* If the memo is unread, */
			if (memo->status == MEMO_NEW)
			{
				/* mark it as read */
				memo->status = MEMO_READ;
				/* and decrease "new memos" count */
				myuser->memoct_new--;
				mysender = myuser_find(memo->sender);

				/* If the sender's inbox is not full and is not MemoServ */
				if ((mysender != NULL) && (mysender->memos.count < me.mdlimit) && strcasecmp(memosvs.nick, memo->sender))
				{       
					receipt = smalloc(sizeof(mymemo_t));
					receipt->sent = CURRTIME;
					receipt->status = MEMO_NEW;
					strlcpy(receipt->sender, memosvs.nick, NICKLEN);
					snprintf(receipt->text, MEMOLEN, "%s has read a memo from you sent at %s", myuser->name, strfbuf);

					/* Send a memo to the sender that the memo was read */
					n = node_create();
					node_add(receipt, n, &mysender->memos);
					mysender->memoct_new++;
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
 *       fault 1 - validation failed
 *       fault 2 - account does not exist
 *       fault 3 - insufficient parameters
 *       fault 4 - trying to ignore oneself
 *       fault 5 - target does not exist
 *       fault 6 - ignore list full
 *       fault 7 - user already ignored
 *       default - success message
 *
 * Side Effects:
 *       ignores user
 */
static int memo_ignore_add(void *conn, int parc, char *parv[])
{
	user_t *user = user_find_named(parv[1]);
	myuser_t *myuser = user->myuser, *mytarget;
	node_t *n, *node;
	char *tmpbuf;
	static char buf[XMLRPC_BUFSIZE] = "";

	*buf = '\0';

	if (parc < 3)
	{
		xmlrpc_generic_error(3, "Insufficient parameters.");
		return 0;
	}

	if (!(myuser = myuser_find(parv[1])))
	{
		xmlrpc_generic_error(2, "Account nonexistent.");
		return 0;
	}

	if (authcookie_validate(parv[0], myuser) == FALSE)
	{
		xmlrpc_generic_error(1, "Authcookie validation failed.");
		return 0;
	}

	/* Sanity check (target is user) */
	if (!strcasecmp(parv[2], myuser->name))
	{       
		xmlrpc_generic_error(4, "You can't ignore yourself.");
		return 0;
	}
	
	/* Check whether target exists */
	if (!(mytarget = myuser_find_ext(parv[2])))
	{       
		xmlrpc_generic_error(5, "Target user nonexistent.");
		return 0;
	}
	
	/* Ignore list is full */
	if (myuser->memo_ignores.count >= MAXMSIGNORES)
	{       
		xmlrpc_generic_error(6, "Ignore list full.");
		return 0;
	}

	LIST_FOREACH(n, myuser->memo_ignores.head)
	{       
		tmpbuf = (char *)n->data;
		
		/* Check whether the user is already being ignored */
		if (!strcasecmp(tmpbuf, parv[2]))
		{       
			xmlrpc_generic_error(7, "That user is already being ignored.");
			return 0;
		}
	}
	
	/* Ignore user */
	tmpbuf = sstrdup(parv[2]);

	node = node_create();
	node_add(tmpbuf, node, &myuser->memo_ignores);

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
 *       fault 1 - validation failed
 *       fault 2 - account does not exist
 *       fault 3 - insufficient parameters
 *       fault 4 - user not being ignored
 *       default - success message
 *
 * Side Effects:
 *       unignores user
 */
static int memo_ignore_delete(void *conn, int parc, char *parv[])
{
	user_t *user = user_find_named(parv[1]);
	myuser_t *myuser = user->myuser;
	node_t *n, *tn;
	char *tmpbuf;
	static char buf[XMLRPC_BUFSIZE] = "";

	*buf = '\0';

	if (parc < 3)
	{
		xmlrpc_generic_error(3, "Insufficient parameters.");
		return 0;
	}

	if (!(myuser = myuser_find(parv[1])))
	{
		xmlrpc_generic_error(2, "Account nonexistent.");
		return 0;
	}

	if (authcookie_validate(parv[0], myuser) == FALSE)
	{
		xmlrpc_generic_error(1, "Authcookie validation failed.");
		return 0;
	}

	LIST_FOREACH_SAFE(n, tn, myuser->memo_ignores.head)
	{       
		tmpbuf = (char *)n->data;
		
		/* User is in the ignore list */
		if (!strcasecmp(tmpbuf, parv[2]))
		{       
			node_del(n, &myuser->memo_ignores);
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
 *       fault 1 - validation failed
 *       fault 2 - account does not exist
 *       fault 3 - insufficient parameters
 *       fault 4 - list was already cleared
 *       default - success message
 *
 * Side Effects:
 *       unignores all users
 */
static int memo_ignore_clear(void *conn, int parc, char *parv[])
{
	user_t *user = user_find_named(parv[1]);
	myuser_t *myuser = user->myuser;
	node_t *n, *tn;
	static char buf[XMLRPC_BUFSIZE] = "";

	*buf = '\0';

	if (parc < 3)
	{
		xmlrpc_generic_error(3, "Insufficient parameters.");
		return 0;
	}

	if (!(myuser = myuser_find(parv[1])))
	{
		xmlrpc_generic_error(2, "Account nonexistent.");
		return 0;
	}

	if (authcookie_validate(parv[0], myuser) == FALSE)
	{
		xmlrpc_generic_error(1, "Authcookie validation failed.");
		return 0;
	}

	if (LIST_LENGTH(&myuser->memo_ignores) == 0)
	{
		xmlrpc_generic_error(4, "No users on the ignore list.");
		return 0;
	}

	LIST_FOREACH_SAFE(n, tn, myuser->memo_ignores.head)
	{       
		free(n->data);
		node_del(n, &myuser->memo_ignores);
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
 *       fault 1 - validation failed
 *       fault 2 - account does not exist
 *       fault 3 - insufficient parameters
 *       fault 4 - list is empty
 *       default - success message
 *
 * Side Effects:
 *       unignores all users
 */
static int memo_ignore_list(void *conn, int parc, char *parv[])
{
	user_t *user = user_find_named(parv[1]);
	myuser_t *myuser = user->myuser;
	node_t *n, *tn;
	uint8_t i = 1;
	char sendbuf[XMLRPC_BUFSIZE - 1] = "", ignorebuf[64] = "";
	static char buf[XMLRPC_BUFSIZE] = "";

	*buf = '\0';

	if (parc < 3)
	{
		xmlrpc_generic_error(3, "Insufficient parameters.");
		return 0;
	}

	if (!(myuser = myuser_find(parv[1])))
	{
		xmlrpc_generic_error(2, "Account nonexistent.");
		return 0;
	}

	if (authcookie_validate(parv[0], myuser) == FALSE)
	{
		xmlrpc_generic_error(1, "Authcookie validation failed.");
		return 0;
	}

	LIST_FOREACH(n, myuser->memo_ignores.head)
	{
		/* provide a list in the format id:user divided by newlines */
		snprintf(ignorebuf, 64, "%d:%s", i, (char *)n->data);
		strncat(sendbuf, ignorebuf, 64);
		i++;
	}

	if (i == 1)
		xmlrpc_generic_error(4, "Ignore list empty.");

	xmlrpc_string(buf, sendbuf);
	xmlrpc_send(1, buf);

	return 0;
}


void _modinit(module_t *m)
{
	xmlrpc_register_method("atheme.memo.send", memo_send);
	xmlrpc_register_method("atheme.memo.delete", memo_delete);
	xmlrpc_register_method("atheme.memo.list", memo_list);
	xmlrpc_register_method("atheme.memo.read", memo_read);
}

void _moddeinit(void)
{
	xmlrpc_unregister_method("atheme.memo.send");
	xmlrpc_unregister_method("atheme.memo.delete");
	xmlrpc_unregister_method("atheme.memo.list");
	xmlrpc_unregister_method("atheme.memo.read");
}
