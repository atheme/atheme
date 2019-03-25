/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2003-2004 E. Will, et al.
 * Copyright (C) 2005-2006 Atheme Project (http://atheme.org/)
 *
 * String matching
 */

#ifndef ATHEME_INC_MATCH_H
#define ATHEME_INC_MATCH_H 1

#include <atheme/attributes.h>
#include <atheme/stdheaders.h>

#ifdef HAVE_LIBPCRE
#  include <pcre.h>
#endif

enum atheme_regex_type
{
	at_posix = 1,
	at_pcre = 2
};

struct atheme_regex
{
	enum atheme_regex_type  type;
	union {
		regex_t         posix;
#ifdef HAVE_LIBPCRE
		pcre *          pcre;
#endif
	} un;
};

/* cidr.c */
int match_ips(const char *mask, const char *address);
int match_cidr(const char *mask, const char *address);

/* match.c */
#define MATCH_RFC1459   0
#define MATCH_ASCII     1

extern int match_mapping;

#define IsLower(c)  ((unsigned char)(c) > 0x5f)
#define IsUpper(c)  ((unsigned char)(c) < 0x60)

#define C_ALPHA 0x00000001U
#define C_DIGIT 0x00000002U
#define C_NICK  0x00000004U
#define C_USER  0x00000008U

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

int ToLower(int);
int ToUpper(int);

int irccasecmp(const char *, const char *);
int ircncasecmp(const char *, const char *, size_t);

void irccasecanon(char *);
void strcasecanon(char *);
void noopcanon(char *);

int match(const char *, const char *);
char *collapse(char *);

/* regex_create() flags */
#define AREGEX_ICASE	1 /* case insensitive */
#define AREGEX_PCRE	2 /* use libpcre engine */
#define AREGEX_KLINE	4 /* XXX for rwatch, match kline */

struct atheme_regex *regex_create(char *pattern, int flags) ATHEME_FATTR_MALLOC;
char *regex_extract(char *pattern, char **pend, int *pflags);
bool regex_match(struct atheme_regex *preg, char *string);
bool regex_destroy(struct atheme_regex *preg);

#endif /* !ATHEME_INC_MATCH_H */
