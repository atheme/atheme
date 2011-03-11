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

Atheme_Internal_List
chanacs(Atheme_ChannelRegistration self)
CODE:
	RETVAL = perl_list_create(&self->chanacs, "Atheme::ChanAcs");
OUTPUT:
	RETVAL


MODULE = Atheme			PACKAGE = Atheme::ChanAcs

Atheme_ChannelRegistration
channel(Atheme_ChanAcs self)
CODE:
	RETVAL = self->mychan;
OUTPUT:
	RETVAL

Atheme_Entity
entity(Atheme_ChanAcs self)
CODE:
	RETVAL = self->entity;
OUTPUT:
	RETVAL

Atheme_Account
user(Atheme_ChanAcs self)
CODE:
	if (!self->entity || self->entity->type != ENT_USER)
		RETVAL = NULL;
	else
		RETVAL = (Atheme_Account)self->entity;
OUTPUT:
	RETVAL

const char *
hostmask(Atheme_ChanAcs self)
CODE:
	RETVAL = self->host;
OUTPUT:
	RETVAL

const char *
namestr(Atheme_ChanAcs self)
CODE:
	if (self->entity)
		RETVAL = self->entity->name;
	else
		RETVAL = self->host;
OUTPUT:
	RETVAL

unsigned int
flags(Atheme_ChanAcs self)
CODE:
	RETVAL = self->level;
OUTPUT:
	RETVAL

const char *
flagstr(Atheme_ChanAcs self)
CODE:
	RETVAL = bitmask_to_flags(self->level);
OUTPUT:
	RETVAL

time_t
modified(Atheme_ChanAcs self)
CODE:
	RETVAL = self->tmodified;
OUTPUT:
	RETVAL

const char *
datestr(Atheme_ChanAcs self)
CODE:
	RETVAL = time_ago(self->tmodified);
OUTPUT:
	RETVAL

