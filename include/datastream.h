/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Datastream query stuff.
 */

#ifndef ATHEME_DATASTREAM_H
#define ATHEME_DATASTREAM_H

extern void sendq_add(connection_t *cptr, char *buf, size_t len);
extern void sendq_add_eof(connection_t *cptr);
extern void sendq_flush(connection_t *cptr);
extern bool sendq_nonempty(connection_t *cptr);
extern void sendq_set_limit(connection_t *cptr, size_t len);

extern int recvq_length(connection_t *cptr);
extern void recvq_put(connection_t *cptr);
extern int recvq_get(connection_t *cptr, char *buf, size_t len);
extern int recvq_getline(connection_t *cptr, char *buf, size_t len);

extern void sendqrecvq_free(connection_t *cptr);

#endif
