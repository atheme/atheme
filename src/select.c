/*
 * atheme-services: A collection of minimalist IRC services
 * select.c: select(2) socket engine.
 *
 * Copyright (c) 2005-2007 Atheme Project (http://www.atheme.org)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "atheme.h"
#include "internal.h"

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

		if (sendq_nonempty(cptr))
			FD_SET(cptr->fd, &writefds);

		else
			FD_SET(cptr->fd, &readfds);
	}
}

/*
 * connection_select()
 *
 * inputs:
 *       delay in milliseconds
 *
 * outputs:
 *       none
 *
 * side effects:
 *       registered sockets and their associated handlers are acted on.
 */
void connection_select(int delay)
{
	int sr;
	node_t *n, *tn;
	connection_t *cptr;
	struct timeval to;

	update_select_sets();
	to.tv_sec = delay / 1000;
	to.tv_usec = delay % 1000 * 1000;

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
				cptr->write_handler(cptr);
			}
		}

		LIST_FOREACH_SAFE(n, tn, connection_list.head)
		{
			cptr = n->data;

			if (FD_ISSET(cptr->fd, &readfds))
			{
				cptr->read_handler(cptr);
			}
		}

		LIST_FOREACH_SAFE(n, tn, connection_list.head)
		{
			cptr = n->data;
			if (cptr->flags & CF_DEAD)
				connection_close(cptr);
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

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
