/*
 * atheme-services: A collection of minimalist IRC services   
 * datastream.c: Efficient handling of streams and packet queues.
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
#include "datastream.h"

#define SENDQSIZE (4096 - 40)

/* sendq struct */
struct sendq {
	mowgli_node_t node;
	int firstused; /* offset of first used byte */
	int firstfree; /* 1 + offset of last used byte */
	char buf[SENDQSIZE];
};

void sendq_add(connection_t * cptr, char *buf, int len)
{
	mowgli_node_t *n;
	struct sendq *sq;
	int l;
	int pos = 0;

	return_if_fail(cptr != NULL);

	if (cptr->flags & (CF_DEAD | CF_SEND_EOF))
	{
		slog(LG_DEBUG, "sendq_add(): attempted to send to fd %d which is already dead", cptr->fd);
		return;
	}

	if (len == 0)
		return;

	if (cptr->sendq_limit != 0 &&
			MOWGLI_LIST_LENGTH(&cptr->sendq) * SENDQSIZE + len > cptr->sendq_limit)
	{
		slog(LG_INFO, "sendq_add(): sendq limit exceeded on connection %s[%d]",
				cptr->name, cptr->fd);
		cptr->flags |= CF_DEAD;
		return;
	}

	if (!sendq_nonempty(cptr))
		connection_setselect_write(cptr, sendq_flush);

	n = cptr->sendq.tail;
	if (n != NULL)
	{
		sq = n->data;
		l = SENDQSIZE - sq->firstfree;
		if (l > len)
			l = len;
		memcpy(sq->buf + sq->firstfree, buf + pos, l);
		sq->firstfree += l;
		pos += l;
		len -= l;
	}

	while (len > 0)
	{
		sq = smalloc(sizeof(struct sendq));
		sq->firstused = sq->firstfree = 0;
		mowgli_node_add(sq, &sq->node, &cptr->sendq);
		l = SENDQSIZE - sq->firstfree;
		if (l > len)
			l = len;
		memcpy(sq->buf + sq->firstfree, buf + pos, l);
		sq->firstfree += l;
		pos += l;
		len -= l;
	}
}

void sendq_add_eof(connection_t * cptr)
{
	return_if_fail(cptr != NULL);

	if (cptr->flags & (CF_DEAD | CF_SEND_EOF))
	{
		slog(LG_DEBUG, "sendq_add(): attempted to send to fd %d which is already dead", cptr->fd);
		return;
	}
	if (!sendq_nonempty(cptr))
		connection_setselect_write(cptr, sendq_flush);
	cptr->flags |= CF_SEND_EOF;
}

void sendq_flush(connection_t * cptr)
{
        mowgli_node_t *n, *tn;
        struct sendq *sq;
        int l;

	return_if_fail(cptr != NULL);

        MOWGLI_ITER_FOREACH_SAFE(n, tn, cptr->sendq.head)
        {
                sq = (struct sendq *)n->data;

                if (sq->firstused == sq->firstfree)
                        break;

                if ((l = write(cptr->fd, sq->buf + sq->firstused, sq->firstfree - sq->firstused)) == -1)
                {
                        if (errno != EAGAIN)
			{
				slog(LG_DEBUG, "sendq_flush(): write error %d (%s) on connection %s[%d]",
						errno, strerror(errno),
						cptr->name, cptr->fd);
				cptr->flags |= CF_DEAD;
			}
                        return;
                }

                sq->firstused += l;
                if (sq->firstused == sq->firstfree)
                {
			if (MOWGLI_LIST_LENGTH(&cptr->sendq) > 1)
			{
                        	mowgli_node_delete(&sq->node, &cptr->sendq);
                        	free(sq);
			}
			else
				/* keep one struct sendq */
				sq->firstused = sq->firstfree = 0;
                }
                else
                        return;
        }
	if (cptr->flags & CF_SEND_EOF)
	{
		/* shut down write end, kill entire connection
		 * only when the other side acknowledges -- jilles */
#ifdef SHUT_WR
		shutdown(cptr->fd, SHUT_WR);
#else
		shutdown(cptr->fd, 1);
#endif
		cptr->flags |= CF_SEND_DEAD;
	}
	connection_setselect_write(cptr, NULL);
}

bool sendq_nonempty(connection_t *cptr)
{
	mowgli_node_t *n;
	struct sendq *sq;

	if (cptr->flags & CF_SEND_DEAD)
		return false;
	if (cptr->flags & CF_SEND_EOF)
		return true;
	n = cptr->sendq.head;
	if (n == NULL)
		return false;
	sq = n->data;
	return sq->firstfree > sq->firstused;
}

void sendq_set_limit(connection_t *cptr, size_t len)
{
	cptr->sendq_limit = len;
}

int recvq_length(connection_t *cptr)
{
	int l = 0;
	mowgli_node_t *n;
	struct sendq *sq;

	MOWGLI_ITER_FOREACH(n, cptr->recvq.head)
	{
		sq = n->data;
		l += sq->firstfree - sq->firstused;
	}
	return l;
}

void recvq_put(connection_t *cptr)
{
	mowgli_node_t *n;
	struct sendq *sq = NULL;
	int l, ll;

	return_if_fail(cptr != NULL);

	if (cptr->flags & (CF_DEAD | CF_SEND_DEAD))
	{
		/* If CF_SEND_DEAD:
		 * The client closed the connection or sent some
		 * data we don't care about, be done with it.
		 * If CF_DEAD:
		 * Connection died earlier, be done with it now.
		 * -- jilles
		 */
		errno = 0;
		connection_close(cptr);
		return;
	}

	n = cptr->recvq.tail;
	if (n != NULL)
	{
		sq = n->data;
		l = SENDQSIZE - sq->firstfree;
		if (l == 0)
			sq = NULL;
	}
	if (sq == NULL)
	{
		sq = smalloc(sizeof(struct sendq));
		sq->firstused = sq->firstfree = 0;
		mowgli_node_add(sq, &sq->node, &cptr->recvq);
		l = SENDQSIZE;
	}
	errno = 0;

	l = read(cptr->fd, sq->buf + sq->firstfree, l);
	if (l == 0 || (l < 0 && errno != EWOULDBLOCK && errno != EAGAIN && errno != EALREADY && errno != EINTR && errno != ENOBUFS))
	{
		if (l == 0)
			slog(LG_DEBUG, "recvq_put(): fd %d closed the connection", cptr->fd);
		else
			slog(LG_DEBUG, "recvq_put(): lost connection on fd %d", cptr->fd);
		connection_close(cptr);
		return;
	}
	else if (l > 0)
		sq->firstfree += l;

	if (cptr->recvq_handler)
	{
		l = recvq_length(cptr);
		do /* call handler until it consumes nothing */
		{
			cptr->recvq_handler(cptr);
			ll = l;
			l = recvq_length(cptr);
		} while (ll != l && l != 0);
	}
	return;
}

int recvq_get(connection_t *cptr, char *buf, int len)
{
	mowgli_node_t *n, *tn;
	struct sendq *sq;
	int l;
	char *p = buf;

	return_val_if_fail(cptr != NULL, 0);

	MOWGLI_ITER_FOREACH_SAFE(n, tn, cptr->recvq.head)
	{
		sq = (struct sendq *)n->data;

		if (sq->firstused == sq->firstfree || len <= 0)
			break;

		l = sq->firstfree - sq->firstused;
		if (l > len)
			l = len;
		memcpy(p, sq->buf + sq->firstused, l);

		p += l;
		len -= l;
		sq->firstused += l;
		if (sq->firstused == sq->firstfree)
		{
			if (MOWGLI_LIST_LENGTH(&cptr->recvq) > 1)
			{
				mowgli_node_delete(&sq->node, &cptr->recvq);
				free(sq);
			}
			else
				/* keep one struct sendq */
				sq->firstused = sq->firstfree = 0;
		}
		else
			return p - buf;
	}
	return p - buf;
}

int recvq_getline(connection_t *cptr, char *buf, int len)
{
	mowgli_node_t *n, *tn;
	struct sendq *sq, *sq2 = NULL;
	int l = 0;
	char *p = buf;
	char *newline = NULL;

	return_val_if_fail(cptr != NULL, 0);

	MOWGLI_ITER_FOREACH(n, cptr->recvq.head)
	{
		sq2 = n->data;
		l += sq2->firstfree - sq2->firstused;
		newline = memchr(sq2->buf + sq2->firstused, '\n', sq2->firstfree - sq2->firstused);
		if (newline != NULL || l >= len)
			break;
	}
	if (newline == NULL && l < len)
		return 0;

	cptr->flags |= CF_NONEWLINE;
	MOWGLI_ITER_FOREACH_SAFE(n, tn, cptr->recvq.head)
	{
		sq = (struct sendq *)n->data;

		if (sq->firstused == sq->firstfree || len <= 0)
			break;

		l = sq->firstfree - sq->firstused;
		if (l > len)
			l = len;
		if (sq == sq2 && l >= newline - sq->buf - sq->firstused + 1)
			cptr->flags &= ~CF_NONEWLINE, l = newline - sq->buf - sq->firstused + 1;
		memcpy(p, sq->buf + sq->firstused, l);

		p += l;
		len -= l;
		sq->firstused += l;
		if (sq->firstused == sq->firstfree)
		{
			if (MOWGLI_LIST_LENGTH(&cptr->recvq) > 1)
			{
				mowgli_node_delete(&sq->node, &cptr->recvq);
				free(sq);
			}
			else
				/* keep one struct sendq */
				sq->firstused = sq->firstfree = 0;
		}
		else
			return p - buf;
	}
	return p - buf;
}

void sendqrecvq_free(connection_t *cptr)
{
	mowgli_node_t *nptr, *nptr2;
	struct sendq *sq;

	MOWGLI_ITER_FOREACH_SAFE(nptr, nptr2, cptr->recvq.head)
	{
		sq = nptr->data;

		mowgli_node_delete(&sq->node, &cptr->recvq);
		free(sq);
	}

	MOWGLI_ITER_FOREACH_SAFE(nptr, nptr2, cptr->sendq.head)
	{
		sq = nptr->data;

		mowgli_node_delete(&sq->node, &cptr->sendq);
		free(sq);
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
