/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 Atheme Project (http://atheme.org/)
 *
 * Datastream query stuff.
 */

#ifndef ATHEME_INC_DATASTREAM_H
#define ATHEME_INC_DATASTREAM_H 1

#include <atheme/stdheaders.h>
#include <atheme/structures.h>

void sendq_add(struct connection *cptr, char *buf, size_t len);
void sendq_add_eof(struct connection *cptr);
void sendq_flush(struct connection *cptr);
bool sendq_nonempty(struct connection *cptr);
void sendq_set_limit(struct connection *cptr, size_t len);

int recvq_length(struct connection *cptr);
void recvq_put(struct connection *cptr);
int recvq_get(struct connection *cptr, char *buf, size_t len);
int recvq_getline(struct connection *cptr, char *buf, size_t len);

void sendqrecvq_free(struct connection *cptr);

#endif /* !ATHEME_INC_DATASTREAM_H */
