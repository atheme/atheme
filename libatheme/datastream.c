/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Datastream stuff.
 *
 * $Id: datastream.c 6355 2006-09-11 15:15:47Z jilles $
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
                                hook_call_event("connection_dead", cptr);
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
}


boolean_t sendq_nonempty(connection_t *cptr)
{
	node_t *n;
	struct sendq *sq;

	n = cptr->sendq.head;
	if (n == NULL)
		return FALSE;
	sq = n->data;
	return sq->firstfree > sq->firstused;
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
