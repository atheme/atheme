MODULE = Atheme			PACKAGE = Atheme::Channel

Atheme_Channel
find(SV * package, const char * name)
CODE:
	RETVAL = channel_find(name);
OUTPUT:
	RETVAL

const char *
name(Atheme_Channel self)
CODE:
	RETVAL = self->name;
OUTPUT:
	RETVAL

const char *
topic(Atheme_Channel self)
CODE:
	RETVAL = self->topic;
OUTPUT:
	RETVAL

Atheme_Internal_List
members(Atheme_Channel self)
CODE:
	RETVAL = perl_list_create(&self->members, "Atheme::ChanUser");
OUTPUT:
	RETVAL


MODULE = Atheme			PACKAGE = Atheme::ChanUser

Atheme_Channel
channel(Atheme_ChanUser self)
CODE:
	RETVAL = self->chan;
OUTPUT:
	RETVAL

Atheme_User
user(Atheme_ChanUser self)
CODE:
	RETVAL = self->user;
OUTPUT:
	RETVAL
