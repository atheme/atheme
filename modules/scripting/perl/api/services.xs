MODULE = Atheme			PACKAGE = Atheme::Service

Atheme_Service
find(SV * package, const char * name)
CODE:
	RETVAL = service_find(name);
OUTPUT:
	RETVAL

void
do_bind_command(Atheme_Service self, Atheme_Command command)
CODE:
	service_bind_command(self, (command_t *)command);

void
unbind_command(Atheme_Service self, Atheme_Command command)
CODE:
	service_unbind_command(self, (command_t *)command);

const char *
name(Atheme_Service self)
CODE:
	RETVAL = self->internal_name;
OUTPUT:
	RETVAL
