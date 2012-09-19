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
#define PERL_INIT_FILE PERL_MODDIR "/lib/init.pl"

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

typedef struct {
	module_t mod;
	char filename[BUFSIZE];
}perl_script_module_t;

static mowgli_heap_t *perl_script_module_heap;

static module_t *do_script_load(const char *filename);
static bool do_script_unload(const char *filename);
static void perl_script_module_unload_handler(module_t *m, module_unload_intent_t intent);

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
	 *
	 * Secondary hack: some linkers do not respect rpath in dlopen(), so we fall back
	 * to some secondary paths where libperl.so may be living.  --nenolod
	 */
	if (!(libperl_handle = dlopen("libperl.so", RTLD_NOW | RTLD_GLOBAL)) &&
	    !(libperl_handle = dlopen("/usr/lib/perl5/core_perl/CORE/libperl.so", RTLD_NOW | RTLD_GLOBAL)) &&
	    !(libperl_handle = dlopen("/usr/lib64/perl5/core_perl/CORE/libperl.so", RTLD_NOW | RTLD_GLOBAL)))
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
static module_t *do_script_load(const char *filename)
{
	/* Remember, this must now be re-entrant. The use of the static
	 * perl_error buffer is still OK, as it's only used immediately after
	 * setting, without control passing from this function.
	 */
	perl_script_module_t *m = mowgli_heap_alloc(perl_script_module_heap);
	mowgli_strlcpy(m->filename, filename, sizeof(m->filename));

	snprintf(perl_error, sizeof(perl_error),  "Unknown error attempting to load perl script %s",
			filename);

	dSP;
	ENTER;

	SAVETMPS;
	PUSHMARK(SP);

	XPUSHs(newRV_noinc((SV*)get_cv("Atheme::Init::load_script", 0)));
	XPUSHs(sv_2mortal(newSVpv(filename, 0)));
	PUTBACK;

	int perl_return_count = call_pv("Atheme::Init::call_wrapper", G_EVAL | G_SCALAR);

	SPAGAIN;

	if (SvTRUE(ERRSV))
	{
		mowgli_strlcpy(perl_error, SvPV_nolen(ERRSV), sizeof(perl_error));
		goto fail;
	}
	if (1 != perl_return_count)
	{
		snprintf(perl_error, sizeof(perl_error), "Script load didn't return a package name");
		goto fail;
	}

	/* load_script should have returned the package name that was just
	 * loaded...
	 */
	const char *packagename = POPp;
	char info_varname[BUFSIZE];
	snprintf(info_varname, BUFSIZE, "%s::Info", packagename);

	/* ... so use that name to grab the script information hash...
	 */
	HV *info_hash = get_hv(info_varname, 0);
	if (!info_hash)
	{
		snprintf(perl_error, sizeof(perl_error), "Couldn't get package info hash %s", info_varname);
		goto fail;
	}

	/* ..., extract the canonical name...
	 */
	SV **name_var = hv_fetch(info_hash, "name", 4, 0);
	if (!name_var)
	{
		snprintf(perl_error, sizeof(perl_error), "Couldn't find canonical name in package info hash");
		goto fail;
	}

	mowgli_strlcpy(m->mod.name, SvPV_nolen(*name_var), sizeof(m->mod.name));

	/* ... and dependency list.
	 */
	SV **deplist_var = hv_fetch(info_hash, "depends", 7, 0);
	/* Not declaring this is legal... */
	if (deplist_var)
	{
		/* ... but having it as anything but an arrayref isn't. */
		if (!SvROK(*deplist_var) || SvTYPE(SvRV(*deplist_var)) != SVt_PVAV)
		{
			snprintf(perl_error, sizeof(perl_error), "$Info::depends must be an array reference");
			goto fail;
		}

		AV *deplist = (AV*)SvRV(*deplist_var);
		I32 len = av_len(deplist);
		/* av_len returns max index, not number of items */
		for (I32 i = 0; i <= len; ++i)
		{
			SV **item = av_fetch(deplist, i, 0);
			if (!item)
				continue;
			const char *dep_name = SvPV_nolen(*item);
			if (!module_request(dep_name))
			{
				snprintf(perl_error, sizeof(perl_error), "Dependent module %s failed to load",
						dep_name);
				goto fail;
			}
			module_t *dep_mod = module_find_published(dep_name);
			mowgli_node_add(dep_mod, mowgli_node_create(), &m->mod.deplist);
			mowgli_node_add(m, mowgli_node_create(), &dep_mod->dephost);
		}
	}

	FREETMPS;
	LEAVE;
	invalidate_object_references();

	/* Now that everything's loaded, do the module housekeeping stuff. */
	m->mod.unload_handler = perl_script_module_unload_handler;
	/* Can't do much better than the address of the module_t here */
	m->mod.address = m;
	m->mod.can_unload = MODULE_UNLOAD_CAPABILITY_OK;
	return (module_t*)m;

fail:
	slog(LG_ERROR, "Failed to load Perl script %s: %s", filename, perl_error);

	if (info_hash)
		SvREFCNT_dec((SV*)info_hash);

	do_script_unload(filename);

	mowgli_heap_free(perl_script_module_heap, m);
	POPs;

	FREETMPS;
	LEAVE;

	invalidate_object_references();

	return NULL;
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
		mowgli_strlcpy(perl_error, SvPV_nolen(ERRSV), sizeof(perl_error));
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
		mowgli_strlcpy(perl_error, SvPV_nolen(ERRSV), sizeof(perl_error));
		POPs;
	}

	FREETMPS;
	LEAVE;

	invalidate_object_references();

	return retval;
}

/*
 * Connect all of the above to OperServ.
 */
static void os_cmd_perl(sourceinfo_t *si, int parc, char *parv[]);

command_t os_perl = { "PERL", N_("Inspect the Perl interpreter"), PRIV_ADMIN, 2, os_cmd_perl, { .path = "oservice/perl" } };

static int conf_loadscript(mowgli_config_file_entry_t *);

static void hook_module_load(hook_module_load_t *data)
{
	struct stat s;
	char buf[BUFSIZE];

	slog(LG_DEBUG, "Perl hook handler trying to load %s", data->name);

	if (data->module)
		return;

	if (0 == stat(data->path, &s))
	{
		data->handled = 1;
		data->module = do_script_load(data->path);
		return;
	}

	snprintf(buf, BUFSIZE, "%s.pl", data->path);
	if (0 == stat(buf, &s))
	{
		data->handled = 1;
		data->module = do_script_load(buf);
		return;
	}

	snprintf(buf, BUFSIZE, "%s/%s.pl", MODDIR, data->name);
	if (0 == stat(buf, &s))
	{
		data->handled = 1;
		data->module = do_script_load(buf);
		return;
	}

	snprintf(buf, BUFSIZE, "%s/%s.pl", MODDIR "/scripts", data->name);
	if (0 == stat(buf, &s))
	{
		data->handled = 1;
		data->module = do_script_load(buf);
		return;
	}

	snprintf(buf, BUFSIZE, "%s/%s", MODDIR, data->name);
	if (0 == stat(buf, &s))
	{
		data->handled = 1;
		data->module = do_script_load(buf);
		return;
	}

	snprintf(buf, BUFSIZE, "%s/%s", MODDIR "/scripts", data->name);
	if (0 == stat(buf, &s))
	{
		data->handled = 1;
		data->module = do_script_load(buf);
		return;
	}
}

void perl_script_module_unload_handler(module_t *m, module_unload_intent_t intent)
{
	perl_script_module_t *pm = (perl_script_module_t *)m;
	do_script_unload(pm->filename);
	mowgli_heap_free(perl_script_module_heap, pm);
}

/*
 * Module startup/shutdown
 */
void _modinit(module_t *m)
{
	perl_script_module_heap = mowgli_heap_create(sizeof(perl_script_module_t), 256, BH_NOW);
	if (!perl_script_module_heap)
	{
		m->mflags |= MODTYPE_FAIL;
		return;
	}

	if (! startup_perl())
	{
		m->mflags |= MODTYPE_FAIL;
		return;
	}

	service_named_bind_command("operserv", &os_perl);

        hook_add_event("module_load");
        hook_add_module_load(hook_module_load);

	add_top_conf("LOADSCRIPT", conf_loadscript);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("operserv", &os_perl);

	shutdown_perl();

	/* Since all our perl pseudo-modules depend on us, we know they'll
	 * all be deallocated before this. No need to clean them up.
	 */
	mowgli_heap_destroy(perl_script_module_heap);
}

/*
 * Actual command handlers.
 */
static void os_cmd_perl(sourceinfo_t *si, int parc, char *parv[])
{
	if (!do_script_list(si))
		command_fail(si, fault_badparams, _("Failed to retrieve script list: %s"), perl_error);
}

static int conf_loadscript(mowgli_config_file_entry_t *ce)
{
	char pathbuf[4096];
	char *name;

	if (!cold_start)
		return 0;

	if (ce->vardata == NULL)
	{
		conf_report_warning(ce, "no parameter for configuration option");
		return 0;
	}

	name = ce->vardata;

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
