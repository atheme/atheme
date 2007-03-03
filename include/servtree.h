/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Headers for service selection tree.
 *
 * $Id: servtree.h 7779 2007-03-03 13:55:42Z pippijn $
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

	void (*handler)(sourceinfo_t *, int, char **);
	void (*notice_handler)(sourceinfo_t *, int, char **);

	list_t *cmdtree;
};

E void servtree_init(void);
E service_t *add_service(char *name, char *user, char *host, char *real,
        void (*handler)(sourceinfo_t *si, int parc, char *parv[]),
        list_t *cmdtree);
E void del_service(service_t *sptr);
E service_t *find_service(char *name);
E char *service_name(char *name);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
