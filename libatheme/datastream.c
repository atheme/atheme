/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Datastream stuff.
 *
 * $Id: datastream.c 6325 2006-09-07 22:39:09Z jilles $
 */
#include <org.atheme.claro.base>

/* sendq struct */
struct sendq {
	char *buf;
	int len;
	int pos;
};

void sendq_add(connection_t * cptr, char *buf, int len, int pos)
{
        node_t *n;
        struct sendq *sq;

	if (!cptr)
		return;

	n = node_create();
	sq = smalloc(sizeof(struct sendq));

        sq->buf = sstrdup(buf);
        sq->len = len - pos;
        sq->pos = pos;
        node_add(sq, n, &cptr->sendq);
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

#ifdef _WIN32
                if ((l = send(cptr->fd, sq->buf + sq->pos, sq->len, 0)) == -1)
#else
                if ((l = write(cptr->fd, sq->buf + sq->pos, sq->len)) == -1)
#endif
                {
                        if (errno != EAGAIN)
                                hook_call_event("connection_dead", cptr);
                        return;
                }

                if (l == sq->len)
                {
                        node_del(n, &cptr->sendq);
                        free(sq->buf);
                        free(sq);
                        node_free(n);
                }
                else
                {
                        sq->pos += l;
                        sq->len -= l;
                        return;
                }
        }
}

void sendqrecvq_free(connection_t *cptr)
{
	node_t *nptr, *nptr2;
	struct sendq *sptr;

	LIST_FOREACH_SAFE(nptr, nptr2, cptr->recvq.head)
	{
		sptr = nptr->data;

		node_del(nptr, &cptr->recvq);
		node_free(nptr);

		free(sptr->buf);
		free(sptr);
	}

	LIST_FOREACH_SAFE(nptr, nptr2, cptr->sendq.head)
	{
		sptr = nptr->data;

		node_del(nptr, &cptr->sendq);
		node_free(nptr);

		free(sptr->buf);
		free(sptr);
	}
}
