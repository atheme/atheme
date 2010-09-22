#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/tabletest", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_tabletest(sourceinfo_t *si, int parc, char *parv[]);

command_t os_tabletest = { "TABLETEST", "Table test.", AC_NONE, 0, os_cmd_tabletest, { .path = "" } };

void _modinit(module_t *m)
{
        service_named_bind_command("operserv", &os_tabletest);
}

void _moddeinit()
{
	service_named_unbind_command("operserv", &os_tabletest);
}

static void os_cmd_tabletest(sourceinfo_t *si, int parc, char *parv[])
{
	table_t *t = table_new("Table \2test\2");
	table_row_t *r = table_row_new(t);

	table_cell_associate(r, "foo", "bar");
	table_cell_associate(r, "F", "-");
	table_cell_associate(r, "baz", "splork");

	r = table_row_new(t);

	table_cell_associate(r, "foo", "1");
	table_cell_associate(r, "F", "+");
	table_cell_associate(r, "baz", "2");

	r = table_row_new(t);

	table_cell_associate(r, "foo", "beagle4");
	table_cell_associate(r, "F", "+");
	table_cell_associate(r, "baz", "boo");

	command_success_table(si, t);

	object_unref(t);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
