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

struct flags_table chanacs_flags[256] = {
	['v'] = {CA_VOICE, 0, true,      "voice"},
	['V'] = {CA_AUTOVOICE, 0, true,  "autovoice"},
	['o'] = {CA_OP, 0, true,         "op"},
	['O'] = {CA_AUTOOP, 0, true,     "autoop"},
	['t'] = {CA_TOPIC, 0, true,      "topic"},
	['s'] = {CA_SET, 0, true,        "set"},
	['r'] = {CA_REMOVE, 0, true,     "remove"},
	['i'] = {CA_INVITE, 0, true,     "invite"},
	['R'] = {CA_RECOVER, 0, true,    "recover"},
	['f'] = {CA_FLAGS, 0, true,      "acl-change"},
	['h'] = {CA_HALFOP, 0, true,     "halfop"},
	['H'] = {CA_AUTOHALFOP, 0, true, "autohalfop"},
	['A'] = {CA_ACLVIEW, 0, true,    "acl-view"},
	['F'] = {CA_FOUNDER, 0, false,   "founder"},
	['q'] = {CA_USEOWNER, 0, true,   "owner"},
	['a'] = {CA_USEPROTECT, 0, true, "protect"},
	['b'] = {CA_AKICK, 0, false,     "banned"},
	['e'] = {CA_EXEMPT, 0, true,     "exempt"},
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
	{ 'N', MU_NEVERGROUP },
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
	{ 'e', MC_VERBOSE_OPS },
	{ 't', MC_TOPICLOCK },
	{ 'g', MC_GUARD },
	{ 'p', MC_PRIVATE },
	{ 'n', MC_NOSYNC },
	{ 0, 0 },
};

struct gflags soper_flags[] = {
	{ 'c', SOPER_CONF },
	{ 0, 0 },
};

unsigned int flags_associate(unsigned char flag, unsigned int restrictflags, bool def, const char *name)
{
	if (chanacs_flags[flag].value && chanacs_flags[flag].value != 0xFFFFFFFF)
		return 0;

	chanacs_flags[flag].value = flags_find_slot();
	chanacs_flags[flag].restrictflags = restrictflags;
	chanacs_flags[flag].def = def;
	chanacs_flags[flag].name = name;

	update_chanacs_flags();

	return chanacs_flags[flag].value;
}

void flags_clear(unsigned char flag)
{
	/* 0xFFFFFFFF = orphaned flag */
	chanacs_flags[flag].value = 0xFFFFFFFF;
	chanacs_flags[flag].restrictflags = 0;
	chanacs_flags[flag].def = false;
	chanacs_flags[flag].name = NULL;
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
	unsigned int flag;
	bool shortflag = false;
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
			  if (!shortflag && (flag = xflag_lookup(string)) != 0x0)
			  {
				  if (status == FLAGS_ADD)
				  {
					  *addflags |= flag;
					  *removeflags &= ~flag;
				  }
				  else if (status == FLAGS_DEL)
				  {
					  *addflags &= ~flag;
					  *removeflags |= flag;
				  }

				  *addflags &= ca_all;
				  *removeflags &= ca_all;

				  return;
			  }
			  else if ((flag = chanacs_flags[(unsigned char)*string].value) != 0x0)
			  {
				  if (status == FLAGS_ADD)
				  {
					  *addflags |= flag;
					  *removeflags &= ~flag;
				  }
				  else if (status == FLAGS_DEL)
				  {
					  *addflags &= ~flag;
					  *removeflags |= flag;
				  }
			  }

			  shortflag = true;
		}

		string++;
	}

	*addflags &= ca_all;
	*removeflags &= ca_all;

	return;
}

unsigned int flags_to_bitmask(const char *string, unsigned int flags)
{
	unsigned int bitmask = (flags ? flags : 0x0);
	int status = FLAGS_ADD;
	unsigned int flag;

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
			  if ((flag = chanacs_flags[(unsigned char) *string].value) != 0x0)
			  {
				  if (status == FLAGS_ADD)
					  bitmask |= flag;
				  else if (status == FLAGS_DEL)
					  bitmask &= ~flag;
			  }
		}

		string++;
	}

	return bitmask & ca_all;
}

char *bitmask_to_flags(unsigned int flags)
{
	char *bptr;
	unsigned int i = 0;

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

unsigned int xflag_lookup(const char *name)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(chanacs_flags); i++)
	{
		if (chanacs_flags[i].name == NULL)
			continue;

		if (!strcasecmp(chanacs_flags[i].name, name))
			return chanacs_flags[i].value;
	}

	return 0;
}

unsigned int xflag_apply(unsigned int in, const char *name)
{
	unsigned int out, flag;
	int status = FLAGS_ADD;

	out = in;

	switch (*name)
	{
	case '+':
		status = FLAGS_ADD;
		name++;
		break;
	case '-':
		status = FLAGS_DEL;
		name++;
		break;
	}

	flag = xflag_lookup(name);

	if (status == FLAGS_ADD)
		out |= flag;
	else
		out &= ~flag;	

	return out;
}

const char *xflag_tostr(unsigned int flags)
{
	unsigned int i;
	static char buf[BUFSIZE];

	*buf = '\0';

	for (i = 0; i < ARRAY_SIZE(chanacs_flags); i++)
	{
		if (chanacs_flags[i].name == NULL)
			continue;

		if (!(flags & chanacs_flags[i].value))
			continue;

		if (*buf != '\0')
			mowgli_strlcat(buf, ", ", sizeof buf);

		mowgli_strlcat(buf, chanacs_flags[i].name, sizeof buf);
	}

	return buf;
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

static bool gflag_fromchar(gflags_t *gflags, char f, unsigned int *res)
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
	*res = 0;
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
