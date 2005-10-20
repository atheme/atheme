/*
 * Copyright (c) 2005 William Pitcock <nenolod@nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Socketengine implementing poll().
 *
 * $Id: poll.c 3053 2005-10-20 18:04:13Z nenolod $
 */

#include <org.atheme.claro.base>
#include <sys/poll.h>

extern list_t connection_list; /* this lives in connection.c */

/*
 * linux does not provide POLLWRNORM by default, and we're not _XOPEN_SOURCE.
 * so.... we have to do this crap below.
 */
#ifndef POLLRDNORM
#define POLLRDNORM POLLIN
#endif
#ifndef POLLWRNORM
#define POLLWRNORM POLLOUT
#endif

struct pollfd pollfds[FD_SETSIZE]; /* XXX We need a define indicating MAXCONN. */

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
	memset(&pollfds, 0, sizeof(struct pollfd) * FD_SETSIZE);
}

/*
 * update_poll_fds()
 *
 * inputs:
 *       none
 *
 * outputs:
 *       none
 *
 * side effects:
 *       registered sockets are prepared for the poll() loop.
 */
static void update_poll_fds(void)
{
	node_t *n;
	connection_t *cptr;
	int slot = 0;

	LIST_FOREACH(n, connection_list.head)
	{
		cptr = n->data;

		cptr->pollslot = slot;

		if (CF_IS_CONNECTING(cptr) || cptr->sendq.count != 0)
		{
			pollfds[slot].fd = cptr->fd;
			pollfds[slot].events |= POLLWRNORM;
			pollfds[slot].revents = 0;
		}
		else
		{
			pollfds[slot].fd = cptr->fd;
			pollfds[slot].events |= POLLRDNORM;
			pollfds[slot].revents = 0;
		}
		slot++;
	}
}

/*
 * connection_select()
 *
 * inputs:
 *       delay in microseconds
 *
 * outputs:
 *       none
 *
 * side effects:
 *       registered sockets and their associated handlers are acted on.
 */
void connection_select(uint32_t delay)
{
	int32_t sr;
	node_t *n;
	connection_t *cptr;

	update_poll_fds();

	if ((sr = poll(pollfds, connection_list.count, delay / 100)) > 0)
	{
		LIST_FOREACH(n, connection_list.head)
		{
			cptr = n->data;

			if (pollfds[cptr->pollslot].revents == 0)
				continue;

			if (pollfds[cptr->pollslot].revents & (POLLRDNORM | POLLIN | POLLHUP | POLLERR))
			{
				if (CF_IS_LISTENING(cptr))
					hook_call_event("listener_in", cptr);
				else
					cptr->read_handler(cptr);
			}

			if (pollfds[cptr->pollslot].revents & (POLLWRNORM | POLLOUT | POLLHUP | POLLERR))
			{
				if (CF_IS_CONNECTING(cptr))
					hook_call_event("connected", cptr);
				else
					cptr->write_handler(cptr);
			}

			pollfds[cptr->pollslot].events = 0;
		}
	}
}
