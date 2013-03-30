#ifndef INLINE_CHANSERV_H
#define INLINE_CHANSERV_H

typedef void (*cs_cmd_proto)(sourceinfo_t*, int, char**);

static inline unsigned int custom_founder_check(void)
{
	char *p;

	if (chansvs.founder_flags != NULL && (p = strchr(chansvs.founder_flags, 'F')) != NULL)
		return flags_to_bitmask(chansvs.founder_flags, 0);
	else
		return CA_INITIAL & ca_all;
}

static inline bool invert_purpose(sourceinfo_t *si, int parc, char *chan, char **nick, char neg, cs_cmd_proto func)
{
	char *negargar[2];

	if ((*nick)[0] != '+' && (*nick)[0] != '-')
		return false;

	if ((*nick)[1] == '+' || (*nick)[1] == '-' || (*nick)[1] == '\0')
	{
		command_fail(si, fault_badparams, _("Not a valid action: \2%s\2"), *nick);
		return true;
	}

	if ((*nick)[0] == (neg != '+' ? '-' : '+'))
	{
		negargar[0] = chan;
		negargar[1] = *nick + 1;
		(*func)(si, parc, negargar);
		return true;
	}
	else if((*nick)[0] == (neg == '+' ? '-' : '+'))
		(*nick)++;
	return false;
}

#endif
