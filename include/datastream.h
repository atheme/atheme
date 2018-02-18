/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Datastream query stuff.
 */

#ifndef ATHEME_DATASTREAM_H
#define ATHEME_DATASTREAM_H

extern void sendq_add(struct connection *cptr, char *buf, size_t len);
extern void sendq_add_eof(struct connection *cptr);
extern void sendq_flush(struct connection *cptr);
extern bool sendq_nonempty(struct connection *cptr);
extern void sendq_set_limit(struct connection *cptr, size_t len);

extern int recvq_length(struct connection *cptr);
extern void recvq_put(struct connection *cptr);
extern int recvq_get(struct connection *cptr, char *buf, size_t len);
extern int recvq_getline(struct connection *cptr, char *buf, size_t len);

extern void sendqrecvq_free(struct connection *cptr);

#endif
