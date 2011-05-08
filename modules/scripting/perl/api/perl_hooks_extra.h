#ifndef PERL_HOOKS_EXTRA_H
#define PERL_HOOKS_EXTRA_H

/*
 * Special-case. Pass undef to the handler routine where it takes no argument.
 */
static void perl_hook_marshal_void(perl_hook_marshal_direction_t dir, void * data, SV ** psv)
{
	if (dir == PERL_HOOK_TO_PERL)
		*psv = &PL_sv_undef;
}

/*
 * Another special case. This hook structure has a union in it, with the type
 * used (and hence the package to which to bless it) dependent on the hook that's
 * being called.
 */
static void perl_hook_marshal_hook_expiry_req_t(perl_hook_marshal_direction_t dir, hook_expiry_req_t * data, SV **psv,
		const char * argname, const char * packagename)
{
	if (dir == PERL_HOOK_TO_PERL)
	{
		HV *hash = newHV();
		SV *sv_tmp = NULL;
		/* Doesn't matter which we pick here; the package name determines what it'll be 
		 * converted to before use. */
		sv_tmp = bless_pointer_to_package(data->data.mc, packagename);
		hv_store(hash, argname, strlen(argname), sv_tmp, 0);
		sv_setiv(sv_tmp, data->do_expire);
		hv_store(hash, "do_expire", 8, sv_tmp, 0);

		*psv = newRV_noinc((SV*)hash);
	} else {
		return_if_fail(SvROK(*psv) && SvTYPE(SvRV(*psv)) == SVt_PVHV);
		HV *hash = (HV*)SvRV(*psv);
		SV **psv_tmp = NULL;
		psv_tmp = hv_fetch(hash, "do_expire", 8, 0);
		data->do_expire = SvIV(*psv_tmp);
	}
}


/*
 * These hook functions need special handling, since they use the same argument
 * structure but need to populate it in different ways.
 */

static void perl_hook_expiry_check(hook_expiry_req_t * data, const char *hookname, const char *packagename, const char * argname)
{
	SV *arg;
	perl_hook_marshal_hook_expiry_req_t(PERL_HOOK_TO_PERL, data, &arg, packagename, argname);

	dSP;
	ENTER;
	SAVETMPS;
	PUSHMARK(SP);

	XPUSHs(sv_2mortal(newRV_noinc((SV*)get_cv("Atheme::Hooks::call_hooks", 0))));
	XPUSHs(sv_2mortal(newSVpv(hookname, 0)));
	XPUSHs(arg);
	call_pv("Atheme::Init::call_wrapper", G_EVAL | G_DISCARD);

	SPAGAIN;
	FREETMPS;
	LEAVE;

	perl_hook_marshal_hook_expiry_req_t(PERL_HOOK_FROM_PERL, data, &arg, NULL, NULL);
	SvREFCNT_dec(arg);
	invalidate_object_references();
}



static void perl_hook_user_check_expire(hook_expiry_req_t * data)
{
	perl_hook_expiry_check(data, "user_check_expire", "Atheme::Account", "account");
}

static void perl_hook_nick_check_expire(hook_expiry_req_t * data)
{
	perl_hook_expiry_check(data, "nick_check_expire", "Atheme::NickRegistration", "nick");
}

static void perl_hook_channel_check_expire(hook_expiry_req_t * data)
{
	perl_hook_expiry_check(data, "channel_check_expire", "Atheme::ChannelRegistration", "channel");
}

#endif
