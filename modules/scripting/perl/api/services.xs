MODULE = Atheme			PACKAGE = Atheme::Service

Atheme_Service
find(SV * package, const char * name)
CODE:
	RETVAL = service_find(name);
OUTPUT:
	RETVAL

Atheme_Service
create(SV * package, const char * name)
CODE:
	RETVAL = service_add(name, NULL);
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

const char *
nick(Atheme_Service self)
CODE:
	RETVAL = self->nick;
OUTPUT:
	RETVAL

const char *
user(Atheme_Service self)
CODE:
	RETVAL = self->user;
OUTPUT:
	RETVAL

const char *
host(Atheme_Service self)
CODE:
	RETVAL = self->host;
OUTPUT:
	RETVAL

const char *
realname(Atheme_Service self)
CODE:
	RETVAL = self->real;
OUTPUT:
	RETVAL

Atheme_User
me(Atheme_Service self)
CODE:
	RETVAL = self->me;
OUTPUT:
	RETVAL

