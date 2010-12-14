/*
 * atheme-services: A collection of minimalist IRC services
 * match.c: Casemapping and matching functions.
 *
 * Copyright (c) 2005-2007 Atheme Project (http://www.atheme.org)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "atheme.h"

#include <regex.h>
#ifdef HAVE_PCRE
#include <pcre.h>
#endif

#define BadPtr(x) (!(x) || (*(x) == '\0'))

int match_mapping = MATCH_RFC1459;

const unsigned char ToLowerTab[] = {
	0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa,
	0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14,
	0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
	0x1e, 0x1f,
	' ', '!', '"', '#', '$', '%', '&', 0x27, '(', ')',
	'*', '+', ',', '-', '.', '/',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	':', ';', '<', '=', '>', '?',
	'@', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
	'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
	't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~',
	'_',
	'`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
	'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
	't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~',
	0x7f,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
	0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9,
	0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9,
	0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9,
	0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9,
	0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
	0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

const unsigned char ToUpperTab[] = {
	0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa,
	0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14,
	0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
	0x1e, 0x1f,
	' ', '!', '"', '#', '$', '%', '&', 0x27, '(', ')',
	'*', '+', ',', '-', '.', '/',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	':', ';', '<', '=', '>', '?',
	'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
	'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
	'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^',
	0x5f,
	'`', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
	'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
	'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^',
	0x7f,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
	0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9,
	0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9,
	0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9,
	0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9,
	0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
	0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

int ToLower(int c)
{
	if (match_mapping == MATCH_ASCII)
		return tolower(c);
	/* else */
	return (ToLowerTab[(unsigned char)(c)]);
}

int ToUpper(int c)
{
	if (match_mapping == MATCH_ASCII)
		return toupper(c);
	/* else */
	return (ToUpperTab[(unsigned char)(c)]);
}

void set_match_mapping(int type)
{
	match_mapping = type;
}

#define MAX_ITERATIONS  512
/*
**  Compare if a given string (name) matches the given
**  mask (which can contain wild cards: '*' - match any
**  number of chars, '?' - match any single character.
**
**      return  0, if match
**              1, if no match
*/

/*
** match()
** Iterative matching function, rather than recursive.
** Written by Douglas A Lewis (dalewis@acsu.buffalo.edu)
*/

int match(const char *mask, const char *name)
{
	const u_char *m = (const u_char *)mask, *n = (const u_char *)name;
	const char *ma = mask, *na = name;
	int wild = 0, q = 0, calls = 0;

	if (!mask || !name)
		return 1;

	/* if the mask is "*", it matches everything */
	if ((*m == '*') && (*(m + 1) == '\0'))
		return 0;

	while (1)
	{
#ifdef  MAX_ITERATIONS
		if (calls++ > MAX_ITERATIONS)
			break;
#endif

		if (*m == '*')
		{
			while (*m == '*')
				m++;
			wild = 1;
			ma = (const char *)m;
			na = (const char *)n;
		}

		if (!*m)
		{
			if (!*n)
				return 0;
			for (m--; (m > (const u_char *) mask) && (*m == '?' || *m == '&' || *m == '#'); m--)
				;
			if ((m > (const u_char *) mask) && (*m == '*') && (m[-1] != '\\'))
				return 0;
			if (!wild)
				return 1;
			m = (const u_char *) ma;
			n = (const u_char *)++ na;
		}
		else if (!*n)
			return 1;
		if ((*m == '\\') && ((m[1] == '*') || (m[1] == '?') || (m[1] == '&') || (m[1] == '#') || (m[1] == '%')))
		{
			m++;
			q = 1;
		}
		else
			q = 0;

		if ((ToLower(*m) != ToLower(*n)) && (((*m != '?') && !(*m == '&' && IsAlpha(*n)) && !(*m == '#' && IsDigit(*n)) && !(*m == '%' && IsNon(*n))) || q))
		{
			if (!wild)
				return 1;
			m = (const u_char *) ma;
			n = (const u_char *)++ na;
		}
		else
		{
			if (*m)
				m++;
			if (*n)
				n++;
		}
	}

	return 1;
}


/*
** collapse a pattern string into minimal components.
** This particular version is "in place", so that it changes the pattern
** which is to be reduced to a "minimal" size.
*/
char *collapse(char *pattern)
{
	char *s = pattern, *s1, *t;

	if (BadPtr(pattern))
		return pattern;
	/*
	 * Collapse all \** into \*, \*[?]+\** into \*[?]+
	 */
	for (; *s; s++)
		if (*s == '\\')
			if (!*(s + 1))
				break;
			else
				s++;
		else if (*s == '*')
		{
			if (*(t = s1 = s + 1) == '*')
				while (*t == '*')
					t++;
			else if (*t == '?')
				for (t++, s1++; *t == '*' || *t == '?'; t++)
					if (*t == '?')
						*s1++ = *t;
			while ((*s1++ = *t++))
				;
		}
	return pattern;
}

/*
**  Case insensitive comparison of two null terminated strings.
**
**      returns  0, if s1 equal to s2
**              <0, if s1 lexicographically less than s2
**              >0, if s1 lexicographically greater than s2
*/
int irccasecmp(const char *s1, const char *s2)
{
	const unsigned char *str1 = (const unsigned char *)s1;
	const unsigned char *str2 = (const unsigned char *)s2;
	int res;

	if (!s1 || !s2)
		return -1;

	if (match_mapping == MATCH_ASCII)
		return strcasecmp(s1, s2);

	while ((res = ToUpper(*str1) - ToUpper(*str2)) == 0)
	{
		if (*str1 == '\0')
			return 0;
		str1++;
		str2++;
	}
	return (res);
}

int ircncasecmp(const char *str1, const char *str2, size_t n)
{
	const unsigned char *s1 = (const unsigned char *)str1;
	const unsigned char *s2 = (const unsigned char *)str2;
	int res;

	if (match_mapping == MATCH_ASCII)
		return strncasecmp(str1, str2, n);

	while ((res = ToUpper(*s1) - ToUpper(*s2)) == 0)
	{
		s1++;
		s2++;
		n--;
		if (n == 0 || (*s1 == '\0' && *s2 == '\0'))
			return 0;
	}
	return (res);
}

void irccasecanon(char *str)
{
	while (*str)
	{
		*str = ToUpper(*str);
		str++;
	}
	return;
}

void strcasecanon(char *str)
{
	while (*str)
	{
		*str = toupper(*str);
		str++;
	}
	return;
}

void noopcanon(char *str)
{
	return;
}

const unsigned int charattrs[] = {
	/* 0  */ 0,
	/* 1  */ 0,
	/* 2  */ 0,
	/* 3  */ 0,
	/* 4  */ 0,
	/* 5  */ 0,
	/* 6  */ 0,
	/* 7 BEL */ 0,
	/* 8  \b */ 0,
	/* 9  \t */ 0,
	/* 10 \n */ 0,
	/* 11 \v */ 0,
	/* 12 \f */ 0,
	/* 13 \r */ 0,
	/* 14 */ 0,
	/* 15 */ 0,
	/* 16 */ 0,
	/* 17 */ 0,
	/* 18 */ 0,
	/* 19 */ 0,
	/* 20 */ 0,
	/* 21 */ 0,
	/* 22 */ 0,
	/* 23 */ 0,
	/* 24 */ 0,
	/* 25 */ 0,
	/* 26 */ 0,
	/* 27 */ 0,
	/* 28 */ 0,
	/* 29 */ 0,
	/* 30 */ 0,
	/* 31 */ 0,
	/* SP */ 0,
	/* ! */ 0,
	/* " */ 0,
	/* # */ 0,
	/* $ */ 0,
	/* % */ 0,
	/* & */ 0,
	/* ' */ 0,
	/* ( */ 0,
	/* ) */ 0,
	/* * */ 0,
	/* + */ 0,
	/* , */ 0,
	/* - */ 0,
	/* . */ 0,
	/* / */ 0,
	/* 0 */ C_DIGIT,
	/* 1 */ C_DIGIT,
	/* 2 */ C_DIGIT,
	/* 3 */ C_DIGIT,
	/* 4 */ C_DIGIT,
	/* 5 */ C_DIGIT,
	/* 6 */ C_DIGIT,
	/* 7 */ C_DIGIT,
	/* 8 */ C_DIGIT,
	/* 9 */ C_DIGIT,
	/* : */ 0,
	/* ; */ 0,
	/* < */ 0,
	/* = */ 0,
	/* > */ 0,
	/* ? */ 0,
	/* @ */ 0,
	/* A */ C_ALPHA,
	/* B */ C_ALPHA,
	/* C */ C_ALPHA,
	/* D */ C_ALPHA,
	/* E */ C_ALPHA,
	/* F */ C_ALPHA,
	/* G */ C_ALPHA,
	/* H */ C_ALPHA,
	/* I */ C_ALPHA,
	/* J */ C_ALPHA,
	/* K */ C_ALPHA,
	/* L */ C_ALPHA,
	/* M */ C_ALPHA,
	/* N */ C_ALPHA,
	/* O */ C_ALPHA,
	/* P */ C_ALPHA,
	/* Q */ C_ALPHA,
	/* R */ C_ALPHA,
	/* S */ C_ALPHA,
	/* T */ C_ALPHA,
	/* U */ C_ALPHA,
	/* V */ C_ALPHA,
	/* W */ C_ALPHA,
	/* X */ C_ALPHA,
	/* Y */ C_ALPHA,
	/* Z */ C_ALPHA,
	/* [ */ 0,
	/* \ */ 0,
	/* ] */ 0,
	/* ^ */ 0,
	/* _ */ 0,
	/* ` */ 0,
	/* a */ C_ALPHA,
	/* b */ C_ALPHA,
	/* c */ C_ALPHA,
	/* d */ C_ALPHA,
	/* e */ C_ALPHA,
	/* f */ C_ALPHA,
	/* g */ C_ALPHA,
	/* h */ C_ALPHA,
	/* i */ C_ALPHA,
	/* j */ C_ALPHA,
	/* k */ C_ALPHA,
	/* l */ C_ALPHA,
	/* m */ C_ALPHA,
	/* n */ C_ALPHA,
	/* o */ C_ALPHA,
	/* p */ C_ALPHA,
	/* q */ C_ALPHA,
	/* r */ C_ALPHA,
	/* s */ C_ALPHA,
	/* t */ C_ALPHA,
	/* u */ C_ALPHA,
	/* v */ C_ALPHA,
	/* w */ C_ALPHA,
	/* x */ C_ALPHA,
	/* y */ C_ALPHA,
	/* z */ C_ALPHA,
	/* { */ 0,
	/* | */ 0,
	/* } */ 0,
	/* ~ */ 0,
	/* del  */ 0,
	/* 0x80 */ 0,
	/* 0x81 */ 0,
	/* 0x82 */ 0,
	/* 0x83 */ 0,
	/* 0x84 */ 0,
	/* 0x85 */ 0,
	/* 0x86 */ 0,
	/* 0x87 */ 0,
	/* 0x88 */ 0,
	/* 0x89 */ 0,
	/* 0x8A */ 0,
	/* 0x8B */ 0,
	/* 0x8C */ 0,
	/* 0x8D */ 0,
	/* 0x8E */ 0,
	/* 0x8F */ 0,
	/* 0x90 */ 0,
	/* 0x91 */ 0,
	/* 0x92 */ 0,
	/* 0x93 */ 0,
	/* 0x94 */ 0,
	/* 0x95 */ 0,
	/* 0x96 */ 0,
	/* 0x97 */ 0,
	/* 0x98 */ 0,
	/* 0x99 */ 0,
	/* 0x9A */ 0,
	/* 0x9B */ 0,
	/* 0x9C */ 0,
	/* 0x9D */ 0,
	/* 0x9E */ 0,
	/* 0x9F */ 0,
	/* 0xA0 */ 0,
	/* 0xA1 */ 0,
	/* 0xA2 */ 0,
	/* 0xA3 */ 0,
	/* 0xA4 */ 0,
	/* 0xA5 */ 0,
	/* 0xA6 */ 0,
	/* 0xA7 */ 0,
	/* 0xA8 */ 0,
	/* 0xA9 */ 0,
	/* 0xAA */ 0,
	/* 0xAB */ 0,
	/* 0xAC */ 0,
	/* 0xAD */ 0,
	/* 0xAE */ 0,
	/* 0xAF */ 0,
	/* 0xB0 */ 0,
	/* 0xB1 */ 0,
	/* 0xB2 */ 0,
	/* 0xB3 */ 0,
	/* 0xB4 */ 0,
	/* 0xB5 */ 0,
	/* 0xB6 */ 0,
	/* 0xB7 */ 0,
	/* 0xB8 */ 0,
	/* 0xB9 */ 0,
	/* 0xBA */ 0,
	/* 0xBB */ 0,
	/* 0xBC */ 0,
	/* 0xBD */ 0,
	/* 0xBE */ 0,
	/* 0xBF */ 0,
	/* 0xC0 */ 0,
	/* 0xC1 */ 0,
	/* 0xC2 */ 0,
	/* 0xC3 */ 0,
	/* 0xC4 */ 0,
	/* 0xC5 */ 0,
	/* 0xC6 */ 0,
	/* 0xC7 */ 0,
	/* 0xC8 */ 0,
	/* 0xC9 */ 0,
	/* 0xCA */ 0,
	/* 0xCB */ 0,
	/* 0xCC */ 0,
	/* 0xCD */ 0,
	/* 0xCE */ 0,
	/* 0xCF */ 0,
	/* 0xD0 */ 0,
	/* 0xD1 */ 0,
	/* 0xD2 */ 0,
	/* 0xD3 */ 0,
	/* 0xD4 */ 0,
	/* 0xD5 */ 0,
	/* 0xD6 */ 0,
	/* 0xD7 */ 0,
	/* 0xD8 */ 0,
	/* 0xD9 */ 0,
	/* 0xDA */ 0,
	/* 0xDB */ 0,
	/* 0xDC */ 0,
	/* 0xDD */ 0,
	/* 0xDE */ 0,
	/* 0xDF */ 0,
	/* 0xE0 */ 0,
	/* 0xE1 */ 0,
	/* 0xE2 */ 0,
	/* 0xE3 */ 0,
	/* 0xE4 */ 0,
	/* 0xE5 */ 0,
	/* 0xE6 */ 0,
	/* 0xE7 */ 0,
	/* 0xE8 */ 0,
	/* 0xE9 */ 0,
	/* 0xEA */ 0,
	/* 0xEB */ 0,
	/* 0xEC */ 0,
	/* 0xED */ 0,
	/* 0xEE */ 0,
	/* 0xEF */ 0,
	/* 0xF0 */ 0,
	/* 0xF1 */ 0,
	/* 0xF2 */ 0,
	/* 0xF3 */ 0,
	/* 0xF4 */ 0,
	/* 0xF5 */ 0,
	/* 0xF6 */ 0,
	/* 0xF7 */ 0,
	/* 0xF8 */ 0,
	/* 0xF9 */ 0,
	/* 0xFA */ 0,
	/* 0xFB */ 0,
	/* 0xFC */ 0,
	/* 0xFD */ 0,
	/* 0xFE */ 0,
	/* 0xFF */ 0,
};

enum atheme_regex_type
{
	at_posix = 1,
	at_pcre = 2
};

struct atheme_regex_
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

/*
 * regex_compile()
 *  Compile a regex of `pattern' and return it.
 */
atheme_regex_t *regex_create(char *pattern, int flags)
{
	static char errmsg[BUFSIZE];
	int errnum;
	atheme_regex_t *preg;
	
	if (pattern == NULL)
	{
		return NULL;
	}
	
	preg = smalloc(sizeof(atheme_regex_t));
	if (flags & AREGEX_PCRE)
	{
#ifdef HAVE_PCRE
		const char *errptr;
		int erroffset;

		preg->un.pcre = pcre_compile(pattern, (flags & AREGEX_ICASE ? PCRE_CASELESS : 0) | PCRE_NO_AUTO_CAPTURE, &errptr, &erroffset, NULL);
		if (preg->un.pcre == NULL)
		{
			slog(LG_ERROR, "regex_match(): %s at offset %d in %s",
					errptr, erroffset, pattern);
			free(preg);
			return NULL;
		}
		preg->type = at_pcre;
#else
		slog(LG_ERROR, "regex_match(): PCRE support is not compiled in");
		free(preg);
		return NULL;
#endif
	}
	else
	{
		errnum = regcomp(&preg->un.posix, pattern, (flags & AREGEX_ICASE ? REG_ICASE : 0) | REG_EXTENDED);

		if (errnum != 0)
		{
			regerror(errnum, &preg->un.posix, errmsg, BUFSIZE);
			slog(LG_ERROR, "regex_match(): %s in %s",
					errmsg, pattern);
			regfree(&preg->un.posix);
			free(preg);
			return NULL;
		}
		preg->type = at_posix;
	}
	
	return preg;
}

char *regex_extract(char *pattern, char **pend, int *pflags)
{
	char c, *p, *p2;
	bool backslash = false;

	c = *pattern;
	if (isalnum(c) || isspace(c) || c == '\\')
		return NULL;
	p = pattern + 1;
	while (*p != c || backslash)
	{
		if (*p == '\0')
			return NULL;
		if (backslash || *p == '\\')
			backslash = !backslash;
		p++;
	}
	p2 = p;
	p++;
	*pflags = 0;
	while (*p != '\0' && *p != ' ')
	{
		if (*p == 'i')
			*pflags |= AREGEX_ICASE;
		else if (*p == 'p')
			*pflags |= AREGEX_PCRE;
		else if (!isalnum(*p))
			return NULL;
		p++;
	}
	*pend = p;
	*p2 = '\0';
	return pattern + 1;
}

/*
 * regex_match()
 *  Internal wrapper API for regex matching.
 *  `preg' is the regex to check with, `string' needs to be checked against.
 *  Returns `true' on match, `false' else.
 */
bool regex_match(atheme_regex_t *preg, char *string)
{
	if (preg == NULL || string == NULL)
	{
		slog(LG_ERROR, "regex_match(): we were given NULL string or pattern, bad!");
		return false;
	}

	switch (preg->type)
	{
		case at_posix:
			return regexec(&preg->un.posix, string, 0, NULL, 0) == 0;
#ifdef HAVE_PCRE
		case at_pcre:
			return pcre_exec(preg->un.pcre, NULL, string, strlen(string), 0, 0, NULL, 0) >= 0;
#endif
		default:
			slog(LG_ERROR, "regex_match(): we were given a pattern of unknown type %d, bad!", preg->type);
			return false;
	}
}

/*
 * regex_destroy()
 *  Perform cleanup with regex `preg', free associated memory.
 */
bool regex_destroy(atheme_regex_t *preg)
{
	switch (preg->type)
	{
		case at_posix:
			regfree(&preg->un.posix);
			break;
#ifdef HAVE_PCRE
		case at_pcre:
			pcre_free(preg->un.pcre);
			break;
#endif
		default:
			slog(LG_ERROR, "regex_destroy(): we were given a pattern of unknown type %d, bad!", preg->type);
			break;
	}
	free(preg);
	return true;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
