/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Headers for service selection tree.
 *
 * $Id: servtree.h 6105 2006-08-18 00:24:46Z jilles $
 */

#ifndef SERVTREE_H
#define SERVTREE_H

struct service_ {
	char *name;
	char *user;
	char *host;
	char *real;
	char *disp;
	char *uid;

	user_t *me;

	void (*handler)(char *, uint8_t, char **);
	void (*notice_handler)(char *, uint8_t, char **);
};

E void servtree_init(void);
E service_t *add_service(char *name, char *user, char *host, char *real,
        void (*handler)(char *origin, uint8_t parc, char *parv[]));
E void del_service(service_t *sptr);
E service_t *find_service(char *name);
E char *service_name(char *name);

#endif
