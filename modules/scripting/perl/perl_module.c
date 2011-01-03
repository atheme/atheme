/*
 * Copyright (c) 2010 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Perl scripting module. Provides a basic interface to load
 * and call into the perl interpreter; the other direction is
 * handled in the perl API module.
 */

#include "api/atheme_perl.h"

#include "conf.h"

#include <dlfcn.h>

DECLARE_MODULE_V1
(
	"scripting/perl", false, _modinit, _moddeinit,
	"$Id$",
	"Atheme Development Group <http://www.atheme.org>"
);

/*
 * Definitions:
 *  PERL_INIT_FILE is the perl script that is used to boot the Atheme interface.
 */
#define PERL_INIT_FILE PREFIX "/modules/scripting/lib/init.pl"

/*
 * External functions:
 *  xs_init is defined in the auto-generated perlxsi.c
 */
extern void xs_init(pTHX);

/*
 * Required static variables:
 */
static PerlInterpreter *my_perl = NULL;
static void *libperl_handle = NULL;
static char *_perl_argv[3] = { "", PERL_INIT_FILE, NULL };
static char **perl_argv = &_perl_argv[0];

static char perl_error[512];

/*
 * Startup and shutdown routines.
 *
 * These deal with starting and stopping the perl interpreter.
 */
static bool startup_perl(void)
{
	/*
	 * Hack: atheme modules (hence our dependent libperl.so) are loaded with
	 * RTLD_LOCAL, meaning that they're not available for later resolution. Perl
	 * extension modules assume that libperl.so is already loaded and available.
	 * Make it so.
	 */
	if (!(libperl_handle = dlopen("libperl.so", RTLD_NOW | RTLD_GLOBAL)))
	{
		slog(LG_INFO, "Couldn't dlopen libperl.so");
		return false;
	}

	int perl_argc = 2;
	char **env = NULL;
	PERL_SYS_INIT3(&perl_argc, &perl_argv, &env);

	if (!(my_perl = perl_alloc()))
	{
		slog(LG_INFO, "Couldn't allocate a perl interpreter.");
		return false;
	}

	PL_perl_destruct_level = 1;
	perl_construct(my_perl);

	PL_origalen = 1;
	int exitstatus = perl_parse(my_perl, xs_init, perl_argc, perl_argv, NULL);
	PL_exit_flags |= PERL_EXIT_DESTRUCT_END;

	if (exitstatus != 0)
	{
		slog(LG_INFO, "Couldn't parse perl startup file: %s", SvPV_nolen(ERRSV));
		return false;
	}

	exitstatus = perl_run(my_perl);

	if (exitstatus != 0)
	{
		slog(LG_INFO, "Couldn't run perl startup file: %s", SvPV_nolen(ERRSV));
		return false;
	}

	invalidate_object_references();

	return true;
}

static void shutdown_perl(void)
{
	PL_perl_destruct_level = 1;
	perl_destruct(my_perl);
	perl_free(my_perl);
	my_perl = 0;
	PERL_SYS_TERM();

	if (libperl_handle)
	{
		dlclose(libperl_handle);
		libperl_handle = NULL;
	}

	free_object_list();
}

/*
 * Implementation functions: load or unload a perl script.
 */
static bool do_script_load(const char *filename)
{
	bool retval = true;

	dSP;
	ENTER;

	SAVETMPS;
	PUSHMARK(SP);

	XPUSHs(newRV_noinc((SV*)get_cv("Atheme::Init::load_script", 0)));
	XPUSHs(sv_2mortal(newSVpv(filename, 0)));
	PUTBACK;

	call_pv("Atheme::Init::call_wrapper", G_EVAL | G_DISCARD);

	SPAGAIN;

	if (SvTRUE(ERRSV))
	{
		retval = false;
		strlcpy(perl_error, SvPV_nolen(ERRSV), sizeof(perl_error));
		POPs;
	}

	FREETMPS;
	LEAVE;

	invalidate_object_references();

	return retval;
}

static bool do_script_unload(const char *filename)
{
	bool retval = true;

	dSP;
	ENTER;

	SAVETMPS;
	PUSHMARK(SP);
	XPUSHs(sv_2mortal(newSVpv(filename, 0)));
	PUTBACK;

	call_pv("Atheme::Init::unload_script", G_EVAL | G_DISCARD);

	SPAGAIN;

	if (SvTRUE(ERRSV))
	{
		retval = false;
		strlcpy(perl_error, SvPV_nolen(ERRSV), sizeof(perl_error));
		POPs;
	}

	FREETMPS;
	LEAVE;

	invalidate_object_references();

	return retval;
}

static bool do_script_list(sourceinfo_t *si)
{
	bool retval = true;

	dSP;
	ENTER;

	SAVETMPS;
	PUSHMARK(SP);

	SV *arg = newSV(0);
	sv_setref_pv(arg, "Atheme::Sourceinfo", si);
	XPUSHs(sv_2mortal(arg));
	PUTBACK;

	call_pv("Atheme::Init::list_scripts", G_EVAL | G_DISCARD);

	SPAGAIN;

	if (SvTRUE(ERRSV))
	{
		retval = false;
		strlcpy(perl_error, SvPV_nolen(ERRSV), sizeof(perl_error));
		POPs;
	}

	FREETMPS;
	LEAVE;

	invalidate_object_references();

	return retval;
}

/*
 * Connect all of the above to OperServ.
 *
 * The following commands are provided:
 * /OS SCRIPT LOAD <filename>
 * /OS SCRIPT UNLOAD <filename>
 * /OS SCRIPT LIST
 */
static void os_cmd_script(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_script_load(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_script_unload(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_script_list(sourceinfo_t *si, int parc, char *parv[]);

command_t os_script = { "SCRIPT", N_("Loads or unloads perl scripts."), PRIV_ADMIN, 2, os_cmd_script };

command_t os_script_load = { "LOAD", N_("Load a named perl script."), PRIV_ADMIN, 2, os_cmd_script_load };
command_t os_script_unload = { "UNLOAD", N_("Unload a loaded script."), PRIV_ADMIN, 2, os_cmd_script_unload };
command_t os_script_list = { "LIST", N_("Shows loaded scripts."), PRIV_ADMIN, 2, os_cmd_script_list };

mowgli_patricia_t *os_script_cmdtree;

static int conf_loadscript(config_entry_t *);

/*
 * Module startup/shutdown
 */
void _modinit(module_t *m)
{
	if (! startup_perl())
	{
		m->mflags |= MODTYPE_FAIL;
		return;
	}

	service_named_bind_command("operserv", &os_script);

	os_script_cmdtree = mowgli_patricia_create(strcasecanon);
	command_add(&os_script_load, os_script_cmdtree);
	command_add(&os_script_unload, os_script_cmdtree);
	command_add(&os_script_list, os_script_cmdtree);

	add_top_conf("LOADSCRIPT", conf_loadscript);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("operserv", &os_script);
	command_delete(&os_script_load, os_script_cmdtree);
	command_delete(&os_script_unload, os_script_cmdtree);
	command_delete(&os_script_list, os_script_cmdtree);

	shutdown_perl();
}

/*
 * Actual command handlers.
 */
static void os_cmd_script(sourceinfo_t *si, int parc, char *parv[])
{
	command_t *c;

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SCRIPT");
		command_fail(si, fault_needmoreparams, _("Syntax: SCRIPT LOAD <filename>|UNLOAD <filename>|LIST"));
		return;
	}

	c = command_find(os_script_cmdtree, parv[0]);
	if (c == NULL)
	{
		command_fail(si, fault_badparams, _("Invalid command. Use \2/%s%s help\2 for a command listing."), (ircd->uses_rcommand == false) ? "msg " : "", si->service->disp);
		return;
	}

	command_exec(si->service, si, c, parc-1, parv+1);
}

static void os_cmd_script_load(sourceinfo_t *si, int parc, char *parv[])
{
	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SCRIPT LOAD");
		command_fail(si, fault_needmoreparams, _("Syntax: SCRIPT LOAD <filename>"));
		return;
	}

	if (!do_script_load(parv[0]))
	{
		command_fail(si, fault_badparams, _("Loading \2%s\2 failed. Error was:"), parv[0]);
		command_fail(si, fault_badparams, "%s", perl_error);
		return;
	}

	logcommand(si, CMDLOG_ADMIN, "SCRIPT LOAD: %s", parv[0]);
	command_success_nodata(si, _("Loaded \2%s\2"), parv[0]);

	if (conf_need_rehash)
	{
		logcommand(si, CMDLOG_ADMIN, "REHASH (MODLOAD)");
		wallops("Rehashing \2%s\2 to complete module load by request of \2%s\2.", config_file, get_oper_name(si));
		if (!conf_rehash())
			command_fail(si, fault_nosuch_target, _("REHASH of \2%s\2 failed. Please correct any errors in the file and try again."), config_file);
	}
}

static void os_cmd_script_unload(sourceinfo_t *si, int parc, char *parv[])
{
	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SCRIPT UNLOAD");
		command_fail(si, fault_needmoreparams, _("Syntax: SCRIPT UNLOAD <filename>"));
		return;
	}

	if (!do_script_unload(parv[0]))
	{
		command_fail(si, fault_badparams, _("Unloading \2%s\2 failed. Error was:"), parv[0]);
		command_fail(si, fault_badparams, "%s", perl_error);
		return;
	}

	logcommand(si, CMDLOG_ADMIN, "SCRIPT UNLOAD: %s", parv[0]);
	command_success_nodata(si, _("Unloaded \2%s\2"), parv[0]);
}

static void os_cmd_script_list(sourceinfo_t *si, int parc, char *parv[])
{
	if (!do_script_list(si))
		command_fail(si, fault_badparams, _("Failed to retrieve script list: %s"), perl_error);
}

static int conf_loadscript(config_entry_t *ce)
{
	char pathbuf[4096];
	char *name;

	if (!cold_start)
		return 0;

	if (ce->ce_vardata == NULL)
	{
		conf_report_warning(ce, "no parameter for configuration option");
		return 0;
	}

	name = ce->ce_vardata;

	if (*name == '/')
	{
		do_script_load(name);
	}
	else
	{
		snprintf(pathbuf, 4096, "%s/%s", MODDIR, name);
		do_script_load(pathbuf);
	}

	return 0;
}

