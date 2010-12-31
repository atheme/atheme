MODULE = Atheme			PACKAGE = Atheme::ChannelRegistration

Atheme_ChannelRegistration
find(SV * package, const char * name)
CODE:
	RETVAL = mychan_find(name);
OUTPUT:
	RETVAL

const char *
name(Atheme_ChannelRegistration self)
CODE:
	RETVAL = self->name;
OUTPUT:
	RETVAL

Atheme_Channel
chan(Atheme_ChannelRegistration self)
CODE:
	RETVAL = self->chan;
OUTPUT:
	RETVAL
