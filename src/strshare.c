/*
 * atheme-services: A collection of minimalist IRC services
 * strshare.c: Shared strings.
 *
 * Copyright (c) 2008 Atheme Project (http://www.atheme.org)
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

mowgli_patricia_t *strshare_dict;

typedef struct
{
	int refcount;
} strshare_t;

void strshare_init(void)
{
	strshare_dict = mowgli_patricia_create(noopcanon);
}

char *strshare_get(const char *str)
{
	strshare_t *ss;

	ss = mowgli_patricia_retrieve(strshare_dict, str);
	if (ss != NULL)
		ss->refcount++;
	else
	{
		ss = smalloc(sizeof(strshare_t) + strlen(str) + 1);
		ss->refcount = 1;
		strcpy((char *)(ss + 1), str);
		mowgli_patricia_add(strshare_dict, (char *)(ss + 1), ss);
	}
	return (char *)(ss + 1);
}

void strshare_unref(char *str)
{
	strshare_t *ss;

	ss = (strshare_t *)str - 1;
	ss->refcount--;
	if (ss->refcount == 0)
	{
		mowgli_patricia_delete(strshare_dict, str);
		free(ss);
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
