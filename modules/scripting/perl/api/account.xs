# Atheme::Account inherits Atheme::Entity, which inherits
# Atheme::Object
#

MODULE = Atheme			PACKAGE = Atheme::Entity

Atheme_Entity
find(SV * package, const char * entityname)
CODE:
	RETVAL = myentity_find(entityname);
OUTPUT:
	RETVAL

const char *
name(Atheme_Entity self)
CODE:
	RETVAL = self->name;
OUTPUT:
	RETVAL

const char *
uid(Atheme_Entity self)
CODE:
	RETVAL = self->id;
OUTPUT:
	RETVAL


MODULE = Atheme			PACKAGE = Atheme::Account

Atheme_Account
find(SV * package, const char * identifier)
CODE:
	RETVAL = myuser_find_ext(identifier);
OUTPUT:
	RETVAL

const char *
email(Atheme_Account self)
CODE:
	RETVAL = self->email;
OUTPUT:
	RETVAL

Atheme_Internal_List
nicks(Atheme_Account self)
CODE:
	RETVAL = perl_list_create(&self->nicks, "Atheme::NickRegistration");
OUTPUT:
	RETVAL

Atheme_Internal_List
chanacs(Atheme_Account self)
CODE:
	RETVAL = perl_list_create(&self->ent.chanacs, "Atheme::ChanAcs");
OUTPUT:
	RETVAL

void
notice(Atheme_Account self, Atheme_Service from, const char * text)
CODE:
	myuser_notice(from->nick, self, "%s", text);

void
vhost(Atheme_Account self, const char *host)
CODE:
	mowgli_node_t *n;
	user_t *u;
	char timestring[16];

	snprintf(timestring, 16, "%lu", (unsigned long)time(NULL));

	metadata_add(self, "private:usercloak", host);
	metadata_add(self, "private:usercloak-timestamp", timestring);
	metadata_add(self, "private:usercloak-assigner", "Perl API");

	MOWGLI_ITER_FOREACH(n, self->logins.head)
	{
        u = n->data;
        user_sethost(nicksvs.me->me, u, host);
	}

time_t
registered(Atheme_Account self)
CODE:
    RETVAL = self->registered;
OUTPUT:
    RETVAL

time_t
last_login(Atheme_Account self)
CODE:
    RETVAL = self->lastlogin;
OUTPUT:
    RETVAL

const char *
last_seen(Atheme_Account self)
CODE:
    if (MOWGLI_LIST_LENGTH(&self->logins) != 0) {
        RETVAL = "now";
    } else {
        static char seen[512];
        time_t last = self->lastlogin;
        struct tm *timeinfo = localtime (&last);
        strftime ( seen, 512, "%b %d %H:%M:%S %Y", timeinfo);
        RETVAL = seen;
    }
OUTPUT:
    RETVAL
