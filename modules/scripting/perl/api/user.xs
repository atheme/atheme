MODULE = Atheme			PACKAGE = Atheme::User

Atheme_User
find(SV * package, const char *nick)
CODE:
	RETVAL = user_find(nick);
OUTPUT:
	RETVAL

const char *
nick(Atheme_User self)
CODE:
	RETVAL = self->nick;
OUTPUT:
	RETVAL

const char *
user(Atheme_User self)
CODE:
	RETVAL = self->user;
OUTPUT:
	RETVAL

const char *
host(Atheme_User self)
CODE:
	RETVAL = self->host;
OUTPUT:
	RETVAL

const char *
vhost(Atheme_User self)
CODE:
	RETVAL = self->vhost;
OUTPUT:
	RETVAL

const char *
ip(Atheme_User self)
CODE:
	RETVAL = self->ip;
OUTPUT:
	RETVAL

const char *
gecos(Atheme_User self)
CODE:
	RETVAL = self->gecos;
OUTPUT:
	RETVAL

const char *
uid(Atheme_User self)
CODE:
	RETVAL = self->uid;
OUTPUT:
	RETVAL

Atheme_Account
account(Atheme_User self)
CODE:
	RETVAL = self->myuser;
OUTPUT:
	RETVAL

Atheme_Internal_List
channels(Atheme_User self)
CODE:
	RETVAL = perl_list_create(&self->channels, "Atheme::ChanUser");
OUTPUT:
	RETVAL

Atheme_Server
server(Atheme_User self)
CODE:
	RETVAL = self->server;
OUTPUT:
	RETVAL

void
notice(Atheme_User self, Atheme_Service from, const char * text)
CODE:
	notice_user_sts(from->me, self, text);
