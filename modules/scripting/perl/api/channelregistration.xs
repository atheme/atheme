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

void
drop(Atheme_ChannelRegistration self)
CODE:
    hook_call_channel_drop (self);

    if (self->chan != NULL && !(self->chan->flags & CHAN_LOG)) {
        part (self->name, chansvs.nick);
    }

    object_unref (self);

void
transfer (Atheme_ChannelRegistration self, Atheme_Sourceinfo si, Atheme_Entity user)
CODE:
	mowgli_node_t *n;

	chanacs_t *ca;

    MOWGLI_ITER_FOREACH(n, self->chanacs.head)
	{
		ca = n->data;
		if (ca->entity != NULL && ca->level & CA_FOUNDER)
			chanacs_modify_simple(ca, CA_FLAGS, CA_FOUNDER);
	}

	self->used = CURRTIME;
	chanacs_change_simple(self, user, NULL, CA_FOUNDER_0, 0, entity(si->smu));

    metadata_delete(self, "private:verify:founderchg:newfounder");
	metadata_delete(self, "private:verify:founderchg:timestamp");

time_t
used_time (Atheme_ChannelRegistration self)
CODE:
    RETVAL = self->used;
OUTPUT:
    RETVAL

unsigned int
flags (Atheme_ChannelRegistration self, unsigned int newflags = 0)
CODE:
    if (items > 1)
        self->flags = newflags;
    RETVAL = self->flags;
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

