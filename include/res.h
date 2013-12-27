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
#define IRCD_MAXNS 10

typedef struct {
  sockaddr_any_t saddr;
  socklen_t saddr_len;
} nsaddr_t;

typedef struct {
  char *h_name;
  nsaddr_t addr;
} dns_reply_t;

typedef struct {
  void *ptr; /* pointer used by callback to identify request */
  void (*callback)(void *vptr, dns_reply_t *reply); /* callback to call */
} dns_query_t;

extern nsaddr_t irc_nsaddr_list[];
extern int irc_nscount;

extern void init_resolver(void);
extern void restart_resolver(void);
extern void delete_resolver_queries(const dns_query_t *);
extern void gethost_byname_type(const char *, dns_query_t *, int);
extern void gethost_byaddr(const sockaddr_any_t *, dns_query_t *);
extern void add_local_domain(char *, size_t);
extern void report_dns_servers(sourceinfo_t *);

#endif
