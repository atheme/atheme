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

Atheme_ChannelRegistration
register (Atheme_Channel self, Atheme_Sourceinfo si, Atheme_Account user)
CODE:
    char *name = self->name;
    mychan_t *mc = mychan_add(name);
    hook_channel_req_t hdata;

    if (mc == NULL) {
        Perl_croak (aTHX_ "Failed to create channel registration for %s", name);
    }

    mc->registered = CURRTIME;
    mc->used = CURRTIME;
    mc->mlock_on |= (CMODE_NOEXT | CMODE_TOPIC);
    if (self->limit == 0)
        mc->mlock_off |= CMODE_LIMIT;
    if (self->key == NULL)
        mc->mlock_off |= CMODE_KEY;
    mc->flags |= config_options.defcflags;

    if ( chanacs_add(mc, entity(user), custom_founder_check(), CURRTIME, entity(si->smu)) == NULL) {
        object_unref (mc);
        mc = NULL;
        Perl_croak (aTHX_ "Failed to create channel access for %s", name);
    }

    hdata.si = si;
    hdata.mc = mc;

    hook_call_channel_register(&hdata);

    RETVAL = mc;
OUTPUT:
    RETVAL

unsigned int
limit (Atheme_Channel self)
CODE:
    RETVAL = self->limit;
OUTPUT:
    RETVAL

const char *
key (Atheme_Channel self)
CODE:
    RETVAL = self->key;
OUTPUT:
    RETVAL

time_t
ts (Atheme_Channel self)
CODE:
    RETVAL = self->ts;
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
