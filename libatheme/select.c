/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Socketengine implementing select().
 *
 * $Id: select.c 5313 2006-05-25 15:18:32Z jilles $
 */

#include <org.atheme.claro.base>

extern list_t connection_list; /* this lives in connection.c */
fd_set readfds, writefds;

/*
 * init_socket_queues()
 *
 * inputs:
 *       none
 *
 * outputs:
 *       none
 *
 * side effects:
 *       when using select, we don't need to do anything here.
 */
void init_socket_queues(void)
{
	return;
}

/*
 * update_select_sets()
 *
 * inputs:
 *       none
 *
 * outputs:
 *       none
 *
 * side effects:
 *       registered sockets are prepared for the select() loop.
 */
static void update_select_sets(void)
{
	node_t *n;
	connection_t *cptr;

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);

	LIST_FOREACH(n, connection_list.head)
	{
		cptr = n->data;

		if (CF_IS_CONNECTING(cptr))
			FD_SET(cptr->fd, &writefds);

		else if (CF_IS_LISTENING(cptr))
			FD_SET(cptr->fd, &readfds);

		if (cptr->sendq.count != 0)
			FD_SET(cptr->fd, &writefds);

		else
			FD_SET(cptr->fd, &readfds);
	}
}

/*
 * connection_select()
 *
 * inputs:
 *       delay in nanoseconds
 *
 * outputs:
 *       none
 *
 * side effects:
 *       registered sockets and their associated handlers are acted on.
 */
void connection_select(uint32_t delay)
{
	int8_t sr;
	node_t *n, *tn;
	connection_t *cptr;
	struct timeval to;

	update_select_sets();
	to.tv_sec = 0;
	to.tv_usec = delay;

	if ((sr = select(claro_state.maxfd + 1, &readfds, &writefds, NULL, &to)) > 0)
	{
		/* Iterate twice, so we don't touch freed memory if
		 * a connection is closed.
		 * -- jilles */
		LIST_FOREACH_SAFE(n, tn, connection_list.head)
		{
			cptr = n->data;

			if (FD_ISSET(cptr->fd, &writefds))
			{
				if (CF_IS_CONNECTING(cptr))
					hook_call_event("connected", cptr);
				else
					cptr->write_handler(cptr);
			}
		}

		LIST_FOREACH_SAFE(n, tn, connection_list.head)
		{
			cptr = n->data;

			if (FD_ISSET(cptr->fd, &readfds))
			{
				if (CF_IS_LISTENING(cptr))
					hook_call_event("listener_in", cptr);
				else
					cptr->read_handler(cptr);
			}
		}
	}
	else
	{
		if (sr == 0)
		{
			/* select() timed out */
		}
		else if ((sr == -1) && (errno == EINTR))
		{
			/* some signal interrupted us, restart select() */
		}
		else if (sr == -1)
		{
			return;
		}
	}
}
