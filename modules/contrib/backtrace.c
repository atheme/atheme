#include <execinfo.h>
#include "atheme.h"

DECLARE_MODULE_V1
(
	"contrib/backtrace", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void __segv_hdl(int whocares)
{
	void *array[256];
	char **strings;
	size_t sz, i;

	sz = backtrace(array, 256);
	strings = backtrace_symbols(array, sz);

	slog(LG_INFO, "---------------- [ CRASH ] -----------------");
	slog(LG_INFO, "%zu stack frames, flags %s", sz, get_conf_opts());
	for (i = 0; i < sz; i++)
		slog(LG_INFO, "#%zu --> %p (%s)", i, array[i], strings[i]);
	slog(LG_INFO, "Report to http://jira.atheme.org/");
	slog(LG_INFO, "--------------------------------------------");
}

void _modinit(module_t *m)
{
	signal(SIGSEGV, __segv_hdl);
}

void _moddeinit(module_unload_intent_t intent)
{
	signal(SIGSEGV, NULL);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
