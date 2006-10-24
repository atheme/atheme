/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Datastream stuff.
 *
 * $Id: datastream.c 6929 2006-10-24 15:30:53Z jilles $
 */
#include <org.atheme.claro.base>

#define SENDQSIZE (4096 - 40)

/* sendq struct */
struct sendq {
	node_t node;
	int firstused; /* offset of first used byte */
	int firstfree; /* 1 + offset of last used byte */
	char buf[SENDQSIZE];
};

void sendq_add(connection_t * cptr, char *buf, int len)
{
	node_t *n;
	struct sendq *sq;
	int l;
	int pos = 0;

	if (!cptr)
		return;
	if (cptr->flags & (CF_DEAD | CF_SEND_EOF))
	{
		clog(LG_DEBUG, "sendq_add(): attempted to send to fd %d which is already dead", cptr->fd);
		return;
	}

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
		node_add(sq, &sq->node, &cptr->sendq);
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
	if (!cptr)
		return;
	if (cptr->flags & (CF_DEAD | CF_SEND_EOF))
	{
		clog(LG_DEBUG, "sendq_add(): attempted to send to fd %d which is already dead", cptr->fd);
		return;
	}
	cptr->flags |= CF_SEND_EOF;
}

void sendq_flush(connection_t * cptr)
{
        node_t *n, *tn;
        struct sendq *sq;
        int l;

	if (!cptr)
		return;

        LIST_FOREACH_SAFE(n, tn, cptr->sendq.head)
        {
                sq = (struct sendq *)n->data;

                if (sq->firstused == sq->firstfree)
                        break;

#ifdef _WIN32
                if ((l = send(cptr->fd, sq->buf + sq->firstused, sq->firstfree - sq->firstused, 0)) == -1)
#else
                if ((l = write(cptr->fd, sq->buf + sq->firstused, sq->firstfree - sq->firstused)) == -1)
#endif
                {
                        if (errno != EAGAIN)
			{
				clog(LG_IOERROR, "sendq_flush(): write error %d (%s) on connection %s[%d]",
						errno, strerror(errno),
						cptr->name, cptr->fd);
				cptr->flags |= CF_DEAD;
			}
                        return;
                }

                sq->firstused += l;
                if (sq->firstused == sq->firstfree)
                {
			if (LIST_LENGTH(&cptr->sendq) > 1)
			{
                        	node_del(&sq->node, &cptr->sendq);
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
}

boolean_t sendq_nonempty(connection_t *cptr)
{
	node_t *n;
	struct sendq *sq;

	if (cptr->flags & CF_SEND_DEAD)
		return FALSE;
	if (cptr->flags & CF_SEND_EOF)
		return TRUE;
	n = cptr->sendq.head;
	if (n == NULL)
		return FALSE;
	sq = n->data;
	return sq->firstfree > sq->firstused;
}

int recvq_length(connection_t *cptr)
{
	int l = 0;
	node_t *n;
	struct sendq *sq;

	LIST_FOREACH(n, cptr->recvq.head)
	{
		sq = n->data;
		l += sq->firstfree - sq->firstused;
	}
	return l;
}

void recvq_put(connection_t *cptr)
{
	node_t *n;
	struct sendq *sq = NULL;
	int l, ll;

	if (!cptr)
		return;
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
		node_add(sq, &sq->node, &cptr->recvq);
		l = SENDQSIZE;
	}
	errno = 0;
#ifdef _WIN32
	l = recv(cptr->fd, sq->buf + sq->firstfree, l, 0);
#else
	l = read(cptr->fd, sq->buf + sq->firstfree, l);
#endif
	if (l == 0 || (l < 0 && errno != EWOULDBLOCK && errno != EAGAIN && errno != EALREADY && errno != EINTR && errno != ENOBUFS))
	{
		if (l == 0)
			clog(LG_INFO, "recvq_put(): fd %d closed the connection", cptr->fd);
		else
			clog(LG_INFO, "recvq_put(): lost connection on fd %d", cptr->fd);
		connection_close(cptr);
		return;
	}
	else if (l > 0)
		sq->firstfree += l;

	clog(LG_DEBUG, "recvq_put(): %d bytes received, %d now in recvq", l, recvq_length(cptr));
	if (cptr->recvq_handler)
	{
		l = recvq_length(cptr);
		do /* call handler until it consumes nothing */
		{
			clog(LG_DEBUG, "recvq_put(): calling handler with %d in recvq", l);
			cptr->recvq_handler(cptr);
			ll = l;
			l = recvq_length(cptr);
		} while (ll != l && l != 0);
	}
	return;
}

int recvq_get(connection_t *cptr, char *buf, int len)
{
	node_t *n, *tn;
	struct sendq *sq;
	int l;
	char *p = buf;

	if (!cptr)
		return 0;

	LIST_FOREACH_SAFE(n, tn, cptr->recvq.head)
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
			if (LIST_LENGTH(&cptr->recvq) > 1)
			{
				node_del(&sq->node, &cptr->recvq);
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
	node_t *n, *tn;
	struct sendq *sq, *sq2 = NULL;
	int l = 0;
	char *p = buf;
	char *newline = NULL;

	if (!cptr)
		return 0;

	LIST_FOREACH(n, cptr->recvq.head)
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
	LIST_FOREACH_SAFE(n, tn, cptr->recvq.head)
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
			if (LIST_LENGTH(&cptr->recvq) > 1)
			{
				node_del(&sq->node, &cptr->recvq);
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
	node_t *nptr, *nptr2;
	struct sendq *sq;

	LIST_FOREACH_SAFE(nptr, nptr2, cptr->recvq.head)
	{
		sq = nptr->data;

		node_del(&sq->node, &cptr->recvq);
		free(sq);
	}

	LIST_FOREACH_SAFE(nptr, nptr2, cptr->sendq.head)
	{
		sq = nptr->data;

		node_del(&sq->node, &cptr->sendq);
		free(sq);
	}
}
