/*
 * atheme-services: A collection of minimalist IRC services   
 * flags.c: Functions to convert a flags table into a bitmask.
 *
 * Copyright (c) 2005-2010 Atheme Project (http://www.atheme.org)           
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

#define FLAGS_ADD       0x1
#define FLAGS_DEL       0x2

unsigned int ca_all = CA_ALL_ALL;
unsigned int ca_all_enable = CA_ALL_ALL;

static char flags_buf[128];

struct flags_table chanacs_flags[255] = {
	['v'] = {CA_VOICE, 0, true},
	['V'] = {CA_AUTOVOICE, 0, true},
	['o'] = {CA_OP, 0, true},
	['O'] = {CA_AUTOOP, 0, true},
	['t'] = {CA_TOPIC, 0, true},
	['s'] = {CA_SET, 0, true},
	['r'] = {CA_REMOVE, 0, true},
	['i'] = {CA_INVITE, 0, true},
	['R'] = {CA_RECOVER, 0, true},
	['f'] = {CA_FLAGS, 0, true},
	['h'] = {CA_HALFOP, 0, true},
	['H'] = {CA_AUTOHALFOP, 0, true},
	['A'] = {CA_ACLVIEW, 0, true},
	['F'] = {CA_FOUNDER, 0, false},
	['q'] = {CA_USEOWNER, 0, true},
	['a'] = {CA_USEPROTECT, 0, true},
	['b'] = {CA_AKICK, 0, false},
};

struct gflags mu_flags[] = {
	{ 'h', MU_HOLD },
	{ 'n', MU_NEVEROP },
	{ 'o', MU_NOOP },
	{ 'W', MU_WAITAUTH },
	{ 's', MU_HIDEMAIL },
	{ 'm', MU_NOMEMO },
	{ 'e', MU_EMAILMEMOS },
	{ 'C', MU_CRYPTPASS },
	{ 'b', MU_NOBURSTLOGIN },
	{ 'E', MU_ENFORCE },
	{ 'P', MU_USE_PRIVMSG },
	{ 'p', MU_PRIVATE },
	{ 'Q', MU_QUIETCHG },
	{ 'g', MU_NOGREET },
	{ 'r', MU_REGNOLIMIT },
	{ 0, 0 },
};

struct gflags mc_flags[] = {
	{ 'h', MC_HOLD },
	{ 'o', MC_NOOP },
	{ 'l', MC_LIMITFLAGS },
	{ 'z', MC_SECURE },
	{ 'v', MC_VERBOSE },
	{ 'r', MC_RESTRICTED },
	{ 'k', MC_KEEPTOPIC },
	{ 't', MC_TOPICLOCK },
	{ 'g', MC_GUARD },
	{ 'p', MC_PRIVATE },
	{ 0, 0 },
};

struct gflags soper_flags[] = {
	{ 'c', SOPER_CONF },
	{ 0, 0 },
};

unsigned int flags_associate(unsigned char flag, unsigned int restrictflags, bool def)
{
	if (chanacs_flags[flag].value && chanacs_flags[flag].value != 0xFFFFFFFF)
		return 0;

	chanacs_flags[flag].value = flags_find_slot();
	chanacs_flags[flag].restrictflags = restrictflags;
	chanacs_flags[flag].def = def;

	update_chanacs_flags();

	return chanacs_flags[flag].value;
}

void flags_clear(unsigned char flag)
{
	/* 0xFFFFFFFF = orphaned flag */
	chanacs_flags[flag].value = 0xFFFFFFFF;
	chanacs_flags[flag].restrictflags = 0;
	chanacs_flags[flag].def = false;
}

unsigned int flags_find_slot(void)
{
	unsigned int flag, i;
	unsigned int all_flags = 0;

	for (i = 0; i < ARRAY_SIZE(chanacs_flags); i++)
		all_flags |= chanacs_flags[i].value;

	for (flag = 1; flag && (all_flags & flag); flag <<= 1);

	return flag;
}

/* Construct bitmasks to be added and removed
 * Postcondition *addflags & *removeflags == 0
 * -- jilles */
void flags_make_bitmasks(const char *string, unsigned int *addflags, unsigned int *removeflags)
{
	int status = FLAGS_ADD;

	*addflags = *removeflags = 0;
	while (*string)
	{
		switch (*string)
		{
		  case '+':
			  status = FLAGS_ADD;
			  break;

		  case '-':
			  status = FLAGS_DEL;
			  break;

		  case '=':
			  *addflags = 0;
			  *removeflags = 0xFFFFFFFF;
			  status = FLAGS_ADD;
			  break;

		  case '*':
			  if (status == FLAGS_ADD)
			  {
				  *addflags |= ca_all_enable;
				  *removeflags |= CA_AKICK;
			  }
			  else if (status == FLAGS_DEL)
			  {
				  *addflags = 0;
				  *removeflags = 0xFFFFFFFF;
			  }
			  break;

		  default:
			  if (chanacs_flags[(unsigned int)*string].value)
			  {
				  if (status == FLAGS_ADD)
				  {
					  *addflags |= chanacs_flags[(unsigned int)*string].value;
					  *removeflags &= ~chanacs_flags[(unsigned int)*string].value;
				  }
				  else if (status == FLAGS_DEL)
				  {
					  *addflags &= ~chanacs_flags[(unsigned int)*string].value;
					  *removeflags |= chanacs_flags[(unsigned int)*string].value;
				  }
			  }
		}

		string++;
	}

	*addflags &= ca_all;
	*removeflags &= ca_all;

	return;
}

unsigned int flags_to_bitmask(const char *string, unsigned int flags)
{
	int bitmask = (flags ? flags : 0x0);
	int status = FLAGS_ADD;

	while (*string)
	{
		switch (*string)
		{
		  case '+':
			  status = FLAGS_ADD;
			  break;

		  case '-':
			  status = FLAGS_DEL;
			  break;

		  case '=':
			  bitmask = 0;
			  status = FLAGS_ADD;
			  break;

		  case '*':
			  if (status == FLAGS_ADD)
				  bitmask |= CA_ALLPRIVS & ca_all_enable & ~CA_FOUNDER;
			  else if (status == FLAGS_DEL)
				  bitmask = 0;
			  break;

		  default:
			  if (chanacs_flags[(unsigned int)*string].value)
			  {
				  if (status == FLAGS_ADD)
					  bitmask |= chanacs_flags[(unsigned int)*string].value;
				  else if (status == FLAGS_DEL)
					  bitmask &= ~chanacs_flags[(unsigned int)*string].value;
			  }
		}

		string++;
	}

	return bitmask;
}

char *bitmask_to_flags(unsigned int flags)
{
	char *bptr;
	unsigned char i = 0;

	bptr = flags_buf;

	*bptr++ = '+';

	for (i = 0; i < ARRAY_SIZE(chanacs_flags); i++)
		if (chanacs_flags[i].value & flags)
			*bptr++ = (char) i;

	*bptr++ = '\0';

	return flags_buf;
}

char *bitmask_to_flags2(unsigned int addflags, unsigned int removeflags)
{
	char *bptr;
	unsigned int i = 0;

	bptr = flags_buf;

	if (removeflags)
	{
		*bptr++ = '-';
		for (i = 0; i < ARRAY_SIZE(chanacs_flags); i++)
			if (chanacs_flags[i].value & removeflags)
				*bptr++ = (char) i;
	}
	if (addflags)
	{
		*bptr++ = '+';
		for (i = 0; i < ARRAY_SIZE(chanacs_flags); i++)
			if (chanacs_flags[i].value & addflags)
				*bptr++ = (char) i;
	}

	*bptr++ = '\0';

	return flags_buf;
}

/* flags a non-founder with +f and these flags is allowed to set -- jilles */
unsigned int allow_flags(mychan_t *mc, unsigned int theirflags)
{
	unsigned int flags;

	flags = theirflags;
	flags &= ~CA_AKICK;
	if (flags & CA_REMOVE)
		flags |= CA_AKICK;
	if (flags & CA_OP)
		flags |= CA_AUTOOP;
	if (flags & CA_HALFOP)
		flags |= CA_AUTOHALFOP;
	if (flags & CA_VOICE)
		flags |= CA_AUTOVOICE;
	if (use_limitflags && mc->flags & MC_LIMITFLAGS)
	{
		if (!(theirflags & (CA_HIGHPRIVS & ~CA_FLAGS)))
			flags &= CA_AKICK;
		else if ((theirflags & CA_HIGHPRIVS) != CA_HIGHPRIVS)
			flags &= ~CA_HIGHPRIVS;
	}
	return flags;
}

void update_chanacs_flags(void)
{
	unsigned int i;

	ca_all = ca_all_enable = 0;
	for (i = 0; i < ARRAY_SIZE(chanacs_flags); i++)
	{
		ca_all |= chanacs_flags[i].value;	
		if (chanacs_flags[i].def == true)
			ca_all_enable |= chanacs_flags[i].value;
	}

	if (!ircd->uses_halfops)
		ca_all &= ~(CA_HALFOP | CA_AUTOHALFOP);
	if (!ircd->uses_protect)
		ca_all &= ~CA_USEPROTECT;
	if (!ircd->uses_owner)
		ca_all &= ~CA_USEOWNER;
}


char *gflags_tostr(gflags_t *gflags, unsigned int flags)
{
	static char buf[257];
	char *p = buf;
	int i;
	*p++ = '+';
	for (i = 0; gflags[i].ch; i++)
		if (flags & gflags[i].value)
			*p++ = gflags[i].ch;
	*p = '\0';
	return buf;
}

bool gflag_fromchar(gflags_t *gflags, char f, unsigned int *res)
{
	int i;
	if (f == '+') return true;
	for (i = 0; gflags[i].ch; i++)
	{
		if (gflags[i].ch != f) continue;
		*res |= gflags[i].value;
		return true;
	}
	return false;
}

bool gflags_fromstr(gflags_t *gflags, const char *f, unsigned int *res)
{
	while (*f)
		if (!gflag_fromchar(gflags, *f++, res))
			return false;
	return true;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
