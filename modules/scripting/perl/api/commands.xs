MODULE = Atheme			PACKAGE = Atheme::Command

Atheme_Command
create(SV * package, const char * name, const char * desc, SV * access, int maxparc, SV * help_path, SV * help_func, SV * handler)
CODE:
	perl_command_t * newcmd = malloc(sizeof(perl_command_t));
	newcmd->command.name = sstrdup(name);
	newcmd->command.desc = sstrdup(desc);
	newcmd->command.access = SvOK(access) ? sstrdup(SvPV_nolen(access)) : NULL;
	*(int*)&newcmd->command.maxparc = maxparc;
	newcmd->command.cmd = perl_command_handler;
	newcmd->command.help.path = SvOK(help_path) ? sstrdup(SvPV_nolen(help_path)) : NULL;

	if (SvOK(help_func))
		newcmd->command.help.func = perl_command_help_func;

	if (!SvROK(handler))
		Perl_croak(aTHX_ "Tried to create a command handler that's not a coderef");

	SvREFCNT_inc(handler);
	newcmd->handler = handler;

	if (SvOK(help_func))
	{
		SvREFCNT_inc(help_func);
		newcmd->help_func = help_func;
	}
	else
		newcmd->help_func = NULL;

	RETVAL = newcmd;
OUTPUT:
	RETVAL

void
DESTROY(Atheme_Command self)
CODE:
	free((void*)self->command.name);
	free((void*)self->command.desc);
	free((void*)self->command.access);
	free((void*)self->command.help.path);

	SvREFCNT_dec(self->handler);
	if (self->help_func)
		SvREFCNT_dec(self->help_func);

	free(self);
