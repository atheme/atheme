/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are as defined in doc/LICENSE.
 *
 * String matching
 *
 */

#ifndef _MATCH_H
#define _MATCH_H

/* cidr.c */
E int match_ips(const char *mask, const char *address);
E int match_cidr(const char *mask, const char *address);

/* match.c */
#define MATCH_RFC1459   0
#define MATCH_ASCII     1

E int match_mapping;

#define IsLower(c)  ((unsigned char)(c) > 0x5f)
#define IsUpper(c)  ((unsigned char)(c) < 0x60)

#define C_ALPHA 0x00000001
#define C_DIGIT 0x00000002

E const unsigned int charattrs[];

#define IsAlpha(c)      (charattrs[(unsigned char) (c)] & C_ALPHA)
#define IsDigit(c)      (charattrs[(unsigned char) (c)] & C_DIGIT)
#define IsAlphaNum(c)   (IsAlpha((c)) || IsDigit((c)))
#define IsNon(c)        (!IsAlphaNum((c)))

E const unsigned char ToLowerTab[];
E const unsigned char ToUpperTab[];

void set_match_mapping(int);

E int ToLower(int);
E int ToUpper(int);

E int irccasecmp(const char *, const char *);
E int ircncasecmp(const char *, const char *, size_t);

E void irccasecanon(char *);
E void strcasecanon(char *);
E void noopcanon(char *);

E int match(const char *, const char *);
E char *collapse(char *);

/* regex_create() flags */
#define AREGEX_ICASE	1 /* case insensitive */
#define AREGEX_PCRE	2 /* use libpcre engine */

typedef struct atheme_regex_ atheme_regex_t;

E atheme_regex_t *regex_create(char *pattern, int flags);
E char *regex_extract(char *pattern, char **pend, int *pflags);
E bool regex_match(atheme_regex_t *preg, char *string);
E bool regex_destroy(atheme_regex_t *preg);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
