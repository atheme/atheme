/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Datastream query stuff.
 *
 * $Id: datastream.h 7467 2007-01-14 03:25:42Z nenolod $
 */
#ifndef __CLARODATASTREAM
#define __CLARODATASTREAM

E void sendq_add(connection_t *cptr, char *buf, int len);
E void sendq_add_eof(connection_t *cptr);
E void sendq_flush(connection_t *cptr);
E boolean_t sendq_nonempty(connection_t *cptr);

E int recvq_length(connection_t *cptr);
E void recvq_put(connection_t *cptr);
E int recvq_get(connection_t *cptr, char *buf, int len);
E int recvq_getline(connection_t *cptr, char *buf, int len);

E void sendqrecvq_free(connection_t *cptr);

#endif
