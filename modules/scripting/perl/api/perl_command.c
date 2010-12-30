
#include <EXTERN.h>
#include <perl.h>

#undef _

#include "atheme.h"
#include "atheme_perl.h"

void perl_command_handler(sourceinfo_t *si, const int parc, char **parv)
{
	perl_command_t * pc = (perl_command_t *) si->command;

	dSP;
	ENTER;
	SAVETMPS;
	PUSHMARK(SP);

	SV *sourceinfo_sv = newSV(0);
	sv_setref_pv(sourceinfo_sv, "Atheme::Sourceinfo", si);
	XPUSHs(sv_2mortal(sourceinfo_sv));

	for (int i = 0; i < parc; ++i)
		XPUSHs(sv_2mortal(newSVpv(parv[i], 0)));

	PUTBACK;
	call_sv(pc->handler, G_VOID | G_DISCARD | G_EVAL);
	SPAGAIN;

	if (SvTRUE(ERRSV))
	{
		command_fail(si, fault_unimplemented, _("Unexpected error occurred: %s"), SvPV_nolen(ERRSV));
		slog(LG_ERROR, "Perl handler for command %s/%s returned error: %s",
				si->service->internal_name, si->command->name, SvPV_nolen(ERRSV));
	}

	PUTBACK;
	FREETMPS;
	LEAVE;
}

void perl_command_help_func(sourceinfo_t *si, const char *subcmd)
{
	command_fail(si, fault_unimplemented, _("Perl help commands not yet implemented"));
}
