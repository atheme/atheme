#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>

/* Take the Atheme definition of this. */
#undef _

#include "atheme.h"
#include "atheme_perl.h"

typedef sourceinfo_t *Atheme_Sourceinfo;
typedef perl_command_t *Atheme_Command;
typedef service_t *Atheme_Service;


MODULE = Atheme			PACKAGE = Atheme

void
request_module_dependency(const char *module_name)
CODE:
	if (module_request(module_name) == false)
		Perl_croak(aTHX_ "Failed to load dependency %s", module_name);

	module_t *mod = module_find(module_name);
	module_t *me = module_find(PERL_MODULE_NAME);

	if (mod != NULL && me != NULL)
	{
		slog(LG_DEBUG, "Atheme::request_module_dependency: adding %s as a dependency to %s",
				PERL_MODULE_NAME, module_name);
		mowgli_node_add(mod, mowgli_node_create(), &me->deplist);
		mowgli_node_add(me, mowgli_node_create(), &mod->dephost);
	}

INCLUDE: services.xs
INCLUDE: sourceinfo.xs
INCLUDE: commands.xs
