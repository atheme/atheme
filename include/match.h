/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are as defined in doc/LICENSE.
 *
 * String matching
 */

#ifndef ATHEME_MATCH_H
#define ATHEME_MATCH_H

#include "sysconf.h"

#include <regex.h>

#ifdef HAVE_PCRE
#include <pcre.h>
#endif

enum atheme_regex_type
{
	at_posix = 1,
	at_pcre = 2
};

struct atheme_regex
{
	enum atheme_regex_type type;
	union
	{
		regex_t posix;
#ifdef HAVE_PCRE
		pcre *pcre;
#endif
	} un;
};

/* cidr.c */
extern int match_ips(const char *mask, const char *address);
extern int match_cidr(const char *mask, const char *address);

/* match.c */
#define MATCH_RFC1459   0
#define MATCH_ASCII     1

extern int match_mapping;

#define IsLower(c)  ((unsigned char)(c) > 0x5f)
#define IsUpper(c)  ((unsigned char)(c) < 0x60)

#define C_ALPHA 0x00000001
#define C_DIGIT 0x00000002
#define C_NICK  0x00000004
#define C_USER  0x00000008

extern const unsigned int charattrs[];

#define IsAlpha(c)      (charattrs[(unsigned char) (c)] & C_ALPHA)
#define IsDigit(c)      (charattrs[(unsigned char) (c)] & C_DIGIT)
#define IsNickChar(c)   (charattrs[(unsigned char) (c)] & C_NICK)
#define IsUserChar(c)   (charattrs[(unsigned char) (c)] & C_USER)
#define IsAlphaNum(c)   (IsAlpha((c)) || IsDigit((c)))
#define IsNon(c)        (!IsAlphaNum((c)))

extern const unsigned char ToLowerTab[];
extern const unsigned char ToUpperTab[];

void set_match_mapping(int);

extern int ToLower(int);
extern int ToUpper(int);

extern int irccasecmp(const char *, const char *);
extern int ircncasecmp(const char *, const char *, size_t);

extern void irccasecanon(char *);
extern void strcasecanon(char *);
extern void noopcanon(char *);

extern int match(const char *, const char *);
extern char *collapse(char *);

/* regex_create() flags */
#define AREGEX_ICASE	1 /* case insensitive */
#define AREGEX_PCRE	2 /* use libpcre engine */
#define AREGEX_KLINE	4 /* XXX for rwatch, match kline */

extern struct atheme_regex *regex_create(char *pattern, int flags);
extern char *regex_extract(char *pattern, char **pend, int *pflags);
extern bool regex_match(struct atheme_regex *preg, char *string);
extern bool regex_destroy(struct atheme_regex *preg);

#endif
