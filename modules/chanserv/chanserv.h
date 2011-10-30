#ifndef INLINE_CHANSERV_H
#define INLINE_CHANSERV_H
static inline unsigned int custom_founder_check(void)
{
	char *p;

	if (chansvs.founder_flags != NULL && (p = strchr(chansvs.founder_flags, 'F')) != NULL)
		return flags_to_bitmask(chansvs.founder_flags, 0);
	else
		return CA_INITIAL & ca_all;
}

#endif
