MODULE = Atheme			PACKAGE = Atheme::Server

Atheme_Server
find(SV * package, const char *name)
CODE:
	RETVAL = server_find(name);
OUTPUT:
	RETVAL

const char *
name(Atheme_Server self)
CODE:
	RETVAL = self->name;
OUTPUT:
	RETVAL

const char *
desc(Atheme_Server self)
CODE:
	RETVAL = self->desc;
OUTPUT:
	RETVAL

const char *
sid(Atheme_Server self)
CODE:
	RETVAL = self->sid;
OUTPUT:
	RETVAL

int
hops(Atheme_Server self)
CODE:
	RETVAL = self->hops;
OUTPUT:
	RETVAL

int
invis(Atheme_Server self)
CODE:
	RETVAL = self->invis;
OUTPUT:
	RETVAL

int
opers(Atheme_Server self)
CODE:
	RETVAL = self->opers;
OUTPUT:
	RETVAL

int
away(Atheme_Server self)
CODE:
	RETVAL = self->away;
OUTPUT:
	RETVAL

time_t
connected(Atheme_Server self)
CODE:
	RETVAL = self->connected_since;
OUTPUT:
	RETVAL

Atheme_Server
uplink(Atheme_Server self)
CODE:
	RETVAL = self->uplink;
OUTPUT:
	RETVAL

Atheme_Internal_List
children(Atheme_Server self)
CODE:
	RETVAL = perl_list_create(&self->children, "Atheme::Server");
OUTPUT:
	RETVAL

Atheme_Internal_List
users(Atheme_Server self)
CODE:
	RETVAL = perl_list_create(&self->userlist, "Atheme::User");
OUTPUT:
	RETVAL
