/* cmd.c - command dispatching */

#include "atheme.h"

static int text_to_parv(char *text, int maxparc, char **parv)
{
	int count = 0;
	char *p;

        if (maxparc == 0)
        	return 0;

	if (!text)
		return 0;

	p = text;
	while (count < maxparc - 1 && (parv[count] = strtok(p, " ")) != NULL)
		count++, p = NULL;

	if ((parv[count] = strtok(p, "")) != NULL)
	{
		p = parv[count];
		while (*p == ' ')
			p++;
		parv[count] = p;
		if (*p != '\0')
		{
			p += strlen(p) - 1;
			while (*p == ' ' && p > parv[count])
				p--;
			p[1] = '\0';
			count++;
		}
	}
	return count;
}

static mowgli_patricia_t *canoncmds;

void cmd_init(void)
{
	canoncmds = mowgli_patricia_create(strcasecanon);
}

void cmd_add(const char *name, cmd_t *cmd)
{
	mowgli_patricia_add(canoncmds, name, cmd);
}

void cmd_del(const char *name)
{
	mowgli_patricia_delete(canoncmds, name);
}

cmd_t *cmd_find(const char *name)
{
	return mowgli_patricia_retrieve(canoncmds, name);
}

void cmd_split(cmd_t *c, char *m, int *parc, char **parv)
{
	*parc = text_to_parv(m, c->maxparc, parv);
}

void cmd_exec(sourceinfo_t *si, cmd_t *c, int parc, char *parv[])
{
	if (si->smu)
		language_set_active(si->smu->language);

	if (has_priv(si, c->access))
	{
		if (si->force_language)
			language_set_active(si->force_language);
		c->func(si, parc, parv);
		language_set_active(NULL);
		return;
	}

	if (has_any_privs(si))
		command_fail(si, fault_noprivs, STR_NO_PRIVILEGE, c->access);
	else
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
	if (si->smu)
		language_set_active(NULL);
}

cmdmap_t *cmdmap_new(void)
{
	cmdmap_t *m = smalloc(sizeof *m);
	m->tree = mowgli_patricia_create(strcasecanon);
	return m;
}

static void free_value(const char *key, void *data, void *priv)
{
	free(data);
}

void cmdmap_free(cmdmap_t *m)
{
	mowgli_patricia_destroy(m->tree, free_value, NULL);
}

void cmdmap_add(cmdmap_t *m, const char *name, const char *cname)
{
	mowgli_patricia_add(m->tree, name, sstrdup(cname));
}

const char *cmdmap_resolve(cmdmap_t *m, const char *name)
{
	return mowgli_patricia_retrieve(m->tree, name);
}
