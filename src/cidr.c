/*
 * atheme-services: A collection of minimalist IRC services   
 * cidr.c: CIDR matching.
 *
 * Most code in this file has been copied from ratbox, src/match.c and
 * src/irc_string.c. It provides CIDR matching for IPv4 and IPv6 without
 * special OS support.
 *
 * Copyright (c) 1996-2002 Hybrid Development Team
 * Copyright (c) 2002-2005 ircd-ratbox development team
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
comp_with_mask(void *addr, void *dest, u_int mask)
{
	if (memcmp(addr, dest, mask / 8) == 0)
	{
		int n = mask / 8;
		int m = ((-1) << (8 - (mask % 8)));
		if (mask % 8 == 0 || (((u_char *) addr)[n] & m) == (((u_char *) dest)[n] & m))
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
inet_pton4(const char *src, u_char *dst)
{
	int saw_digit, octets, ch;
	u_char tmp[INADDRSZ], *tp;

	saw_digit = 0;
	octets = 0;
	*(tp = tmp) = 0;
	while((ch = *src++) != '\0')
	{

		if(ch >= '0' && ch <= '9')
		{
			u_int new = *tp * 10 + (ch - '0');

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
inet_pton6(const char *src, u_char *dst)
{
	static const char xdigits[] = "0123456789abcdef";
	u_char tmp[IN6ADDRSZ], *tp, *endp, *colonp;
	const char *curtok;
	int ch, saw_xdigit;
	u_int val;

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
	while((ch = tolower(*src++)) != '\0')
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
			*tp++ = (u_char) (val >> 8) & 0xff;
			*tp++ = (u_char) val & 0xff;
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
		else
			continue;
		return (0);
	}
	if(saw_xdigit)
	{
		if(tp + INT16SZ > endp)
			return (0);
		*tp++ = (u_char) (val >> 8) & 0xff;
		*tp++ = (u_char) val & 0xff;
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
int match_ips(const char *s1, const char *s2)
{
	unsigned char ipaddr[IN6ADDRSZ], maskaddr[IN6ADDRSZ];
	char ipmask[BUFSIZE];
	char ip[HOSTLEN + 1];
	char *len;
	int cidrlen;

	strlcpy(ipmask, s1, sizeof ipmask);
	strlcpy(ip, s2, sizeof ip);

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
	char address[NICKLEN + USERLEN + HOSTLEN + 6];
	char *ipmask;
	char *ip;
	char *len;
	int cidrlen;

	strlcpy(mask, s1, sizeof mask);
	strlcpy(address, s2, sizeof address);

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

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
