/*
 * include/irc_reslib.h
 *
 * $Id: reslib.h 446 2006-02-12 02:46:54Z db $
 */

#ifndef CHARYBDIS_RESLIB_H
#define CHARYBDIS_RESLIB_H

/* Longest hostname we're willing to work with.
 * Due to DNSBLs this is more than HOSTLEN.
 */
#define IRCD_RES_HOSTLEN 255

/* Here we define some values lifted from nameser.h */
#define NS_NOTIFY_OP 4
#define NS_INT16SZ 2
#define NS_IN6ADDRSZ    16
#define NS_INADDRSZ      4
#define NS_INT32SZ 4
#define NS_CMPRSFLGS    0xc0
#define NS_MAXCDNAME 255
#define QUERY 0
#define IQUERY 1
#define NO_ERRORS 0
#define SERVFAIL 2
#define NXDOMAIN 3
#define T_A 1
#define T_AAAA 28
#define T_PTR 12
#define T_CNAME 5
#define T_NULL 10
#define C_IN 1
#define QFIXEDSZ 4
#define RRFIXEDSZ 10
#define HFIXEDSZ 12

typedef struct
{
        unsigned        id :16;         /* query identification number */
#ifdef WORDS_BIGENDIAN
                        /* fields in third byte */
        unsigned        qr: 1;          /* response flag */
        unsigned        opcode: 4;      /* purpose of message */
        unsigned        aa: 1;          /* authoritive answer */
        unsigned        tc: 1;          /* truncated message */
        unsigned        rd: 1;          /* recursion desired */
                        /* fields in fourth byte */
        unsigned        ra: 1;          /* recursion available */
        unsigned        unused :1;      /* unused bits (MBZ as of 4.9.3a3) */
        unsigned        ad: 1;          /* authentic data from named */
        unsigned        cd: 1;          /* checking disabled by resolver */
        unsigned        rcode :4;       /* response code */
#else
                        /* fields in third byte */
        unsigned        rd :1;          /* recursion desired */
        unsigned        tc :1;          /* truncated message */
        unsigned        aa :1;          /* authoritive answer */
        unsigned        opcode :4;      /* purpose of message */
        unsigned        qr :1;          /* response flag */
                        /* fields in fourth byte */
        unsigned        rcode :4;       /* response code */
        unsigned        cd: 1;          /* checking disabled by resolver */
        unsigned        ad: 1;          /* authentic data from named */
        unsigned        unused :1;      /* unused bits (MBZ as of 4.9.3a3) */
        unsigned        ra :1;          /* recursion available */
#endif
                        /* remaining bytes */
        unsigned        qdcount :16;    /* number of question entries */
        unsigned        ancount :16;    /* number of answer entries */
        unsigned        nscount :16;    /* number of authority entries */
        unsigned        arcount :16;    /* number of resource entries */
} RESHEADER;

/*
 * Inline versions of get/put short/long.  Pointer is advanced.
 */
#define IRC_NS_GET16(s, cp) { \
	const unsigned char *t_cp = (const unsigned char *)(cp); \
	(s) = ((uint16_t)t_cp[0] << 8) \
	    | ((uint16_t)t_cp[1]) \
	    ; \
	(cp) += NS_INT16SZ; \
}

#define IRC_NS_GET32(l, cp) { \
	const unsigned char *t_cp = (const unsigned char *)(cp); \
	(l) = ((uint32_t)t_cp[0] << 24) \
	    | ((uint32_t)t_cp[1] << 16) \
	    | ((uint32_t)t_cp[2] << 8) \
	    | ((uint32_t)t_cp[3]) \
	    ; \
	(cp) += NS_INT32SZ; \
}

#define IRC_NS_PUT16(s, cp) { \
	uint16_t t_s = (uint16_t)(s); \
	unsigned char *t_cp = (unsigned char *)(cp); \
	*t_cp++ = t_s >> 8; \
	*t_cp   = t_s; \
	(cp) += NS_INT16SZ; \
}

#define IRC_NS_PUT32(l, cp) { \
	uint32_t t_l = (uint32_t)(l); \
	unsigned char *t_cp = (unsigned char *)(cp); \
	*t_cp++ = t_l >> 24; \
	*t_cp++ = t_l >> 16; \
	*t_cp++ = t_l >> 8; \
	*t_cp   = t_l; \
	(cp) += NS_INT32SZ; \
}

extern int irc_res_init(void);
extern int irc_dn_expand(const unsigned char *msg, const unsigned char *eom, const unsigned char *src, char *dst, int dstsiz);
extern int irc_dn_skipname(const unsigned char *ptr, const unsigned char *eom);
extern unsigned int irc_ns_get16(const unsigned char *src);
extern unsigned long irc_ns_get32(const unsigned char *src);
extern void irc_ns_put16(unsigned int src, unsigned char *dst);
extern void irc_ns_put32(unsigned long src, unsigned char *dst);
extern int irc_res_mkquery(const char *dname, int class, int type, unsigned char *buf, int buflen);

extern char irc_domain[IRCD_RES_HOSTLEN + 1];

#endif
