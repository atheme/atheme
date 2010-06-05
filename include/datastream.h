/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Datastream query stuff.
 *
 */
#ifndef __CLARODATASTREAM
#define __CLARODATASTREAM

E void sendq_add(connection_t *cptr, char *buf, int len);
E void sendq_add_eof(connection_t *cptr);
E void sendq_flush(connection_t *cptr);
E bool sendq_nonempty(connection_t *cptr);
E void sendq_set_limit(connection_t *cptr, size_t len);

E int recvq_length(connection_t *cptr);
E void recvq_put(connection_t *cptr);
E int recvq_get(connection_t *cptr, char *buf, int len);
E int recvq_getline(connection_t *cptr, char *buf, int len);

E void sendqrecvq_free(connection_t *cptr);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
