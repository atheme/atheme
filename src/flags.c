/*
 * Copyright (c) 2005 Atheme Development Group
 * flags.c: Functions to convert a flags table into a bitmask.
 *
 * See doc/LICENSE for licensing information.
 */

#include "atheme.h"

static char flags_buf[128];

struct flags_table chanacs_flags[] = {
	{'v', CA_VOICE},
	{'V', CA_AUTOVOICE},
	{'o', CA_OP},
	{'O', CA_AUTOOP},
	{'t', CA_TOPIC},
	{'s', CA_SET},
	{'r', CA_REMOVE},
	{'i', CA_INVITE},
	{'R', CA_RECOVER},
	{'f', CA_FLAGS},
	{'h', CA_HALFOP},
	{'H', CA_AUTOHALFOP},
	{'A', CA_ACLVIEW},
	{'b', CA_AKICK},
	{0, 0}
};

/* Construct bitmasks to be added and removed
 * Postcondition *addflags & *removeflags == 0
 * -- jilles */
void flags_make_bitmasks(const char *string, struct flags_table table[], uint32_t *addflags, uint32_t *removeflags)
{
	int status = FLAGS_ADD;
	short i = 0;

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
				  *addflags = 0xFFFFFFFF;
				  *removeflags = 0;

				  /* If this is chanacs_flags[], remove the ban flag. */
				  if (table == chanacs_flags)
				  {
					  *addflags &= CA_ALLPRIVS;
					  *removeflags |= CA_AKICK;
				  }
			  }
			  else if (status == FLAGS_DEL)
			  {
				  *addflags = 0;
				  *removeflags = 0xFFFFFFFF;
			  }
			  break;

		  default:
			  for (i = 0; table[i].flag; i++)
				  if (table[i].flag == *string)
				  {
					  if (status == FLAGS_ADD)
					  {
						  *addflags |= table[i].value;
						  *removeflags &= ~table[i].value;
					  }
					  else if (status == FLAGS_DEL)
					  {
						  *addflags &= ~table[i].value;
						  *removeflags |= table[i].value;
					  }
				  }
		}

		*string++;
	}

	return;
}

uint32_t flags_to_bitmask(const char *string, struct flags_table table[], uint32_t flags)
{
	int bitmask = (flags ? flags : 0x0);
	int status = FLAGS_ADD;
	short i = 0;

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
			  {
				  bitmask = 0xFFFFFFFF;

				  /* If this is chanacs_flags[], do privs only */
				  if (table == chanacs_flags)
					  bitmask &= CA_ALLPRIVS;
			  }
			  else if (status == FLAGS_DEL)
				  bitmask = 0;
			  break;

		  default:
			  for (i = 0; table[i].flag; i++)
				  if (table[i].flag == *string)
				  {
					  if (status == FLAGS_ADD)
						  bitmask |= table[i].value;
					  else if (status == FLAGS_DEL)
						  bitmask &= ~table[i].value;
				  }
		}

		*string++;
	}

	return bitmask;
}

char *bitmask_to_flags(uint32_t flags, struct flags_table table[])
{
	char *bptr;
	short i = 0;

	bptr = flags_buf;

	*bptr++ = '+';

	for (i = 0; table[i].flag; i++)
		if (table[i].value & flags)
			*bptr++ = table[i].flag;

	*bptr++ = '\0';

	return flags_buf;
}

char *bitmask_to_flags2(uint32_t addflags, uint32_t removeflags, struct flags_table table[])
{
	char *bptr;
	short i = 0;

	bptr = flags_buf;

	if (removeflags)
	{
		*bptr++ = '-';
		for (i = 0; table[i].flag; i++)
			if (table[i].value & removeflags)
				*bptr++ = table[i].flag;
	}
	if (addflags)
	{
		*bptr++ = '+';
		for (i = 0; table[i].flag; i++)
			if (table[i].value & addflags)
				*bptr++ = table[i].flag;
	}

	*bptr++ = '\0';

	return flags_buf;
}

/* flags a non-founder with +f and these flags is allowed to set -- jilles */
uint32_t allow_flags(uint32_t flags)
{
	flags &= ~CA_AKICK;
	if (flags & CA_REMOVE)
		flags |= CA_AKICK;
	if (flags & CA_OP)
		flags |= CA_AUTOOP;
	if (flags & CA_HALFOP)
		flags |= CA_AUTOHALFOP;
	if (flags & CA_VOICE)
		flags |= CA_AUTOVOICE;
	return flags;
}
