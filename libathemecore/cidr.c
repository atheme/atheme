/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 1996-2002 Hybrid Development Team
 * Copyright (C) 2002-2005 ircd-ratbox development team
 * Copyright (C) 2005-2015 Atheme Project (http://atheme.org/)
 * Copyright (C) 2016-2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * atheme-services: A collection of minimalist IRC services
 * cidr.c: CIDR matching.
 *
 * Most code in this file has been copied from ratbox, src/match.c and
 * src/irc_string.c. It provides CIDR matching for IPv4 and IPv6 without
 * special OS support.
 */

#include <atheme.h>
#include "internal.h"

#ifndef INADDRSZ
#define INADDRSZ 4
#endif
#ifndef IN6ADDRSZ
#define IN6ADDRSZ 16
#endif
#ifndef INT16SZ
#define INT16SZ 2
#endif

/* compares the first 'mask' bits
 * returns 1 if equal, 0 if not */
static int
comp_with_mask(void *addr, void *dest, unsigned int mask)
{
	if (memcmp(addr, dest, mask / 8) == 0)
	{
		int n = mask / 8;
		int m = ((-1) << (8 - (mask % 8)));
		if (mask % 8 == 0 || (((unsigned char *) addr)[n] & m) == (((unsigned char *) dest)[n] & m))
		{
			return (1);
		}
	}
	return (0);
}

/*
 * inet_pton4() and inet_pton6() are
 * Copyright (c) 1996-1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/*
 * WARNING: Don't even consider trying to compile this on a system where
 * sizeof(int) < 4.  sizeof(int) > 4 is fine; all the world's not a VAX.
 */

/* int
 * inet_pton4(src, dst)
 *	like inet_aton() but without all the hexadecimal and shorthand.
 * return:
 *	1 if `src' is a valid dotted quad, else 0.
 * notice:
 *	does not touch `dst' unless it's returning 1.
 * author:
 *	Paul Vixie, 1996.
 */
static int
inet_pton4(const char *src, unsigned char *dst)
{
	int saw_digit, octets, ch;
	unsigned char tmp[INADDRSZ], *tp;

	saw_digit = 0;
	octets = 0;
	*(tp = tmp) = 0;
	while((ch = *src++) != '\0')
	{

		if(ch >= '0' && ch <= '9')
		{
			unsigned int new = *tp * 10 + (ch - '0');

			if(new > 255)
				return (0);
			*tp = new;
			if(!saw_digit)
			{
				if(++octets > 4)
					return (0);
				saw_digit = 1;
			}
		}
		else if(ch == '.' && saw_digit)
		{
			if(octets == 4)
				return (0);
			*++tp = 0;
			saw_digit = 0;
		}
		else
			return (0);
	}
	if(octets < 4)
		return (0);
	memcpy(dst, tmp, INADDRSZ);
	return (1);
}

/* int
 * inet_pton6(src, dst)
 *	convert presentation level address to network order binary form.
 * return:
 *	1 if `src' is a valid [RFC1884 2.2] address, else 0.
 * notice:
 *	(1) does not touch `dst' unless it's returning 1.
 *	(2) :: in a full address is silently ignored.
 * credit:
 *	inspired by Mark Andrews.
 * author:
 *	Paul Vixie, 1996.
 */
static int
inet_pton6(const char *src, unsigned char *dst)
{
	static const char xdigits[] = "0123456789abcdef";
	unsigned char tmp[IN6ADDRSZ], *tp, *endp, *colonp;
	const char *curtok;
	int ch, saw_xdigit;
	unsigned int val;

	tp = memset(tmp, '\0', IN6ADDRSZ);
	endp = tp + IN6ADDRSZ;
	colonp = NULL;
	/* Leading :: requires some special handling. */
	if(*src == ':')
		if(*++src != ':')
			return (0);
	curtok = src;
	saw_xdigit = 0;
	val = 0;
	while((ch = tolower((unsigned char)*src++)) != '\0')
	{
		const char *pch;

		pch = strchr(xdigits, ch);
		if(pch != NULL)
		{
			val <<= 4;
			val |= (pch - xdigits);
			if(val > 0xffff)
				return (0);
			saw_xdigit = 1;
			continue;
		}
		if(ch == ':')
		{
			curtok = src;
			if(!saw_xdigit)
			{
				if(colonp)
					return (0);
				colonp = tp;
				continue;
			}
			else if(*src == '\0')
			{
				return (0);
			}
			if(tp + INT16SZ > endp)
				return (0);
			*tp++ = (unsigned char) (val >> 8) & 0xff;
			*tp++ = (unsigned char) val & 0xff;
			saw_xdigit = 0;
			val = 0;
			continue;
		}
		if(*src != '\0' && ch == '.')
		{
			if(((tp + INADDRSZ) <= endp) && inet_pton4(curtok, tp) > 0)
			{
				tp += INADDRSZ;
				saw_xdigit = 0;
				break;	/* '\0' was seen by inet_pton4(). */
			}
		}
		return (0);
	}
	if(saw_xdigit)
	{
		if(tp + INT16SZ > endp)
			return (0);
		*tp++ = (unsigned char) (val >> 8) & 0xff;
		*tp++ = (unsigned char) val & 0xff;
	}
	if(colonp != NULL)
	{
		/*
		 * Since some memmove()'s erroneously fail to handle
		 * overlapping regions, we'll do the shift by hand.
		 */
		const int n = tp - colonp;
		int i;

		if(tp == endp)
			return (0);
		for(i = 1; i <= n; i++)
		{
			endp[-i] = colonp[n - i];
			colonp[n - i] = 0;
		}
		tp = endp;
	}
	if(tp != endp)
		return (0);
	memcpy(dst, tmp, IN6ADDRSZ);
	return (1);
}

/*
 * match_ips()
 *
 * Input - cidr ip mask, address
 * Output - 0 = Matched 1 = Did not match
 * switched 0 and 1 to be consistent with atheme's match() -- jilles
 */
int
match_ips(const char *s1, const char *s2)
{
	unsigned char ipaddr[IN6ADDRSZ], maskaddr[IN6ADDRSZ];
	char ipmask[BUFSIZE];
	char ip[HOSTLEN + 1];
	char *len;
	int cidrlen;

	if (s1 == NULL || s2 == NULL)
		return 1;

	mowgli_strlcpy(ipmask, s1, sizeof ipmask);
	mowgli_strlcpy(ip, s2, sizeof ip);

	len = strrchr(ipmask, '/');
	if (len == NULL)
		return 1;

	*len++ = '\0';

	cidrlen = atoi(len);
	if (cidrlen == 0)
		return 1;

	if (strchr(ip, ':') && strchr(ipmask, ':'))
	{
		if (cidrlen > 128)
			return 1;
		if (!inet_pton6(ip, ipaddr))
			return 1;
		if (!inet_pton6(ipmask, maskaddr))
			return 1;
		return !comp_with_mask(ipaddr, maskaddr, cidrlen);
	}
	else if (!strchr(ip, ':') && !strchr(ipmask, ':'))
	{
		if (cidrlen > 32)
			return 1;
		if (!inet_pton4(ip, ipaddr))
			return 1;
		if (!inet_pton4(ipmask, maskaddr))
			return 1;
		return !comp_with_mask(ipaddr, maskaddr, cidrlen);
	}
	else
		return 1;
}

/* match_cidr()
 *
 * Input - mask n!u@i/c, address n!u@i
 * Output - 0 = Matched 1 = Did not match
 * switched 0 and 1 to be consistent with atheme's match() -- jilles
 */
int
match_cidr(const char *s1, const char *s2)
{
	unsigned char ipaddr[IN6ADDRSZ], maskaddr[IN6ADDRSZ];
	char mask[BUFSIZE];
	char address[NICKLEN + USERLEN + HOSTLEN + 9];
	char *ipmask;
	char *ip;
	char *len;
	int cidrlen;

	return_val_if_fail(s1 != NULL, 1);
	return_val_if_fail(s2 != NULL, 1);

	mowgli_strlcpy(mask, s1, sizeof mask);
	mowgli_strlcpy(address, s2, sizeof address);

	ipmask = strrchr(mask, '@');
	if (ipmask == NULL)
		return 1;

	*ipmask++ = '\0';

	ip = strrchr(address, '@');
	if (ip == NULL)
		return 1;
	*ip++ = '\0';

	len = strrchr(ipmask, '/');
	if (len == NULL)
		return 1;

	*len++ = '\0';

	cidrlen = atoi(len);
	if (cidrlen == 0)
		return 1;

	if (strchr(ip, ':') && strchr(ipmask, ':'))
	{
		if (cidrlen > 128)
			return 1;
		if (!inet_pton6(ip, ipaddr))
			return 1;
		if (!inet_pton6(ipmask, maskaddr))
			return 1;
		return !comp_with_mask(ipaddr, maskaddr, cidrlen) ||
			match(mask, address);
	}
	else if (!strchr(ip, ':') && !strchr(ipmask, ':'))
	{
		if (cidrlen > 32)
			return 1;
		if (!inet_pton4(ip, ipaddr))
			return 1;
		if (!inet_pton4(ipmask, maskaddr))
			return 1;
		return !comp_with_mask(ipaddr, maskaddr, cidrlen) ||
			match(mask, address);
	}
	else
		return 1;
}

int
valid_ip_or_mask(const char *src)
{
	char ipaddr[HOSTLEN + 7];
	unsigned char buf[IN6ADDRSZ];
	char *mask, *end;
	unsigned long cidrlen;

	if (mowgli_strlcpy(ipaddr, src, sizeof ipaddr) >= sizeof ipaddr)
		return 0;

	int is_ipv6 = (strchr(ipaddr, ':') != NULL);

	if ((mask = strchr(ipaddr, '/')))
	{
		*mask++ = '\0';

		if (!isdigit((unsigned char)*mask))
			return 0;

		cidrlen = strtoul(mask, &end, 10);
		if (*end != '\0')
			return 0;

		if (cidrlen > (is_ipv6 ? 128 : 32))
			return 0;
	}

	if (is_ipv6)
		return inet_pton6(ipaddr, buf);
	else
		return inet_pton4(ipaddr, buf);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
