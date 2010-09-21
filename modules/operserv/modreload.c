#include "atheme.h"
#include "conf.h"

DECLARE_MODULE_V1
(
	"operserv/modreload", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);
static void os_cmd_modreload(sourceinfo_t *si, int parc, char *parv[]);

command_t os_modreload =
{
	"MODRELOAD",
	N_("Reloads a module."),
	PRIV_ADMIN,
	20,
	os_cmd_modreload
};

list_t *os_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

	service_named_bind_command("operserv", &os_modreload);
	help_addentry(os_helptree, "MODRELOAD", "help/oservice/modreload", NULL);
}

void _moddeinit(void)
{
	service_named_unbind_command("operserv", &os_modreload);
	help_delentry(os_helptree, "MODRELOAD");
}

static void os_cmd_modreload(sourceinfo_t *si, int parc, char *parv[])
{
	char *module = parv[0];
	module_t *t, *m;
	char buf[BUFSIZE + 1];

	if (parc < 1)
        {
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MODRELOAD");
		command_fail(si, fault_needmoreparams, _("Syntax: MODRELOAD <module...>"));
		return;
        }

	if (!(m = module_find_published(module)))
	{
		command_fail(si, fault_nochange, _("\2%s\2 is not loaded."), module);
		return;
	}

	if (m->header->norestart)
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is a permanent module; it cannot be reloaded."), module);
		slog(LG_ERROR, "MODRELOAD:ERROR: \2%s\2 tried to reload permanent module \2%s\2", get_oper_name(si), module);
		return;
	}

	if (!strcmp(m->header->name, "operserv/main") || !strcmp(m->header->name, "operserv/modload") || !strcmp(m->header->name, "operserv/modunload") || !strcmp(m->header->name, "operserv/modreload"))
	{
		command_fail(si, fault_noprivs, _("Refusing to reload \2%s\2."), module);
		return;
	}
	module_unload(m);

	if (*module != '/')
	{
		snprintf(buf, BUFSIZE, "%s/%s", MODDIR "/modules", module);
		t = module_load(buf);
	}
	else
		t = module_load(module);

	if (t != NULL)
	{
		logcommand(si, CMDLOG_ADMIN, "MODRELOAD: \2%s\2", module);
		command_success_nodata(si, _("Module \2%s\2 reloaded."), module);
	}
	else
	{
		command_fail(si, fault_nosuch_target, _("Module \2%s\2 failed to reload."), module);
		slog(LG_ERROR, "MODRELOAD:ERROR: \2%s\2 tried to reload \2%s\2, operation failed.", get_oper_name(si), module);
		return;
	}
}
