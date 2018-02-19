/*
 * res.h for referencing functions in res.c, reslib.c
 *
 * $Id: res.h 2023 2006-09-02 23:47:27Z jilles $
 */

#ifndef CHARYBDIS_RES_H
#define CHARYBDIS_RES_H

#include "atheme.h"
#include "reslib.h"

/* Maximum number of nameservers in /etc/resolv.conf we care about
 * In hybrid, this was 2 -- but in Charybdis, we want to track
 * a few more than that ;) --nenolod
 */
#define ATHEME_MAX_NAMESERVERS 10

struct nsaddr
{
  union sockaddr_any saddr;
  socklen_t saddr_len;
};

struct res_dns_reply
{
  char *h_name;
  struct nsaddr addr;
};

struct res_dns_query
{
  void *ptr; /* pointer used by callback to identify request */
  void (*callback)(void *vptr, struct res_dns_reply *reply); /* callback to call */
};

extern struct nsaddr irc_nsaddr_list[ATHEME_MAX_NAMESERVERS];
extern int irc_nscount;

extern void init_resolver(void);
extern void restart_resolver(void);
extern void delete_resolver_queries(const struct res_dns_query *);
extern void gethost_byname_type(const char *, struct res_dns_query *, int);
extern void gethost_byaddr(const union sockaddr_any *, struct res_dns_query *);
extern void add_local_domain(char *, size_t);
extern void report_dns_servers(struct sourceinfo *);

#endif
