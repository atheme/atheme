/* groupserv - group services
 * Copyright (c) 2010 Atheme Development Group
 */

#include "atheme.h"
#include "groupserv.h"

DECLARE_MODULE_V1
(
	"groupserv/main", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

service_t *groupsvs;
list_t gs_cmdtree;
list_t gs_helptree;
list_t conf_gs_table;

static void groupserv(sourceinfo_t *si, int parc, char *parv[])
{
	char *cmd;
	char *text;
	char orig[BUFSIZE];

	if (parv[0][0] == '&')
	{
		slog(LG_ERROR, "services(): got parv with local channel: %s", parv[0]);
		return;
	}

	strlcpy(orig, parv[parc - 1], BUFSIZE);
	cmd = strtok(parv[parc - 1], " ");
	text = strtok(NULL, "");

	if (!cmd) return;
	if (*cmd == '\001')
	{
		handle_ctcp_common(si, cmd, text);
		return;
	}

	command_exec_split(si->service, si, cmd, text, &gs_cmdtree);
}

void _modinit(module_t *m)
{
	mygroups_init();
	groupsvs = service_add("groupserv", groupserv, &gs_cmdtree, &conf_gs_table);
	add_uint_conf_item("MAXGROUPS", &conf_gs_table, 0, &maxgroups, 0, 65535, 0);
	add_uint_conf_item("MAXGROUPACS", &conf_gs_table, 0, &maxgroupacs, 0, 65535, 0);

	gs_db_init();
	gs_hooks_init();
	basecmds_init();
	set_init();
}

void _moddeinit(void)
{
	gs_db_deinit();
	gs_hooks_deinit();
	basecmds_deinit();
	set_deinit();
	del_conf_item("MAXGROUPS", &conf_gs_table);
	del_conf_item("MAXGROUPACS", &conf_gs_table);

	if (groupsvs)
		service_delete(groupsvs);

	mygroups_deinit();
}
