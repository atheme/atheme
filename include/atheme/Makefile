include ../../extra.mk

.PHONY: serno.h

DISTCLEAN	= hooktypes.h serno.h sysconf.h
SUBDIRS		= digest inline protocol

pre-depend: ${DISTCLEAN}

INCLUDES = \
	abirev.h		\
	account.h		\
	argon2.h		\
	attributes.h		\
	auth.h			\
	authcookie.h		\
	base64.h		\
	channels.h		\
	commandhelp.h		\
	commandtree.h		\
	common.h		\
	conf.h			\
	confprocess.h		\
	connection.h		\
	constants.h		\
	crypto.h		\
	culture.h		\
	database_backend.h	\
	datastream.h		\
	digest.h		\
	entity-validation.h	\
	entity.h		\
	flags.h			\
	global.h		\
	hook.h			\
	hooktypes.h		\
	httpd.h			\
	i18n.h			\
	inline.h		\
	libathemecore.h		\
	linker.h		\
	match.h			\
	memory.h		\
	module.h		\
	object.h		\
	pbkdf2.h		\
	phandler.h		\
	pmodule.h		\
	privs.h			\
	random.h		\
	sasl.h			\
	serno.h			\
	servers.h		\
	services.h		\
	servtree.h		\
	sharedheap.h		\
	sourceinfo.h		\
	stdheaders.h		\
	string.h		\
	structures.h		\
	sysconf.h		\
	table.h			\
	taint.h			\
	template.h		\
	tools.h			\
	uid.h			\
	uplink.h		\
	users.h

hooktypes.h: hooktypes.in mkhooktypes.sh
	$(info Generate: $@)
	@sh mkhooktypes.sh hooktypes.in >hooktypes.h

serno.h:
	@sh mkserno.sh

includesubdir = $(PACKAGE)

include ../../buildsys.mk
