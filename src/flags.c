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

		  case '*':
			  if (status == FLAGS_ADD)
			  {
				  bitmask |= 0xFFFFFFFF;

				  /* If this is chanacs_flags[], remove the ban flag. */
				  if (table == chanacs_flags)
				  	bitmask &= ~CA_AKICK;
			  }					
			  else if (status == FLAGS_DEL)
				  bitmask &= ~0xFFFFFFFF;
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
