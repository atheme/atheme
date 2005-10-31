/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Datastream stuff.
 *
 * $Id: datastream.c 3321 2005-10-31 02:01:47Z jilles $
 */
#include <org.atheme.claro.base>

void sendq_add(connection_t * cptr, char *buf, int len, int pos)
{
        node_t *n = node_create();
        struct sendq *sq = smalloc(sizeof(struct sendq));

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
