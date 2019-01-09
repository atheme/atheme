include ../extra.mk

.PHONY: serno.h

DISTCLEAN	= hooktypes.h serno.h sysconf.h
SUBDIRS		= inline protocol

pre-depend: ${DISTCLEAN}

INCLUDES = \
	abirev.h		\
	account.h		\
	atheme.h		\
	atheme_memory.h		\
	atheme_random.h		\
	atheme_string.h		\
	auth.h			\
	authcookie.h		\
	base64.h		\
	channels.h		\
	commandtree.h		\
	common.h		\
	conf.h			\
	confprocess.h		\
	connection.h		\
	crypto.h		\
	culture.h		\
	database_backend.h	\
	datastream.h		\
	digest.h		\
	digest_fe_internal.h	\
	digest_fe_mbedtls.h	\
	digest_fe_nettle.h	\
	digest_fe_openssl.h	\
	entity-validation.h	\
	entity.h		\
	flags.h			\
	global.h		\
	hook.h			\
	hooktypes.h		\
	httpd.h			\
	i18n.h			\
	libathemecore.h		\
	linker.h		\
	match.h			\
	module.h		\
	object.h		\
	phandler.h		\
	pmodule.h		\
	privs.h			\
	sasl.h			\
	serno.h			\
	servers.h		\
	services.h		\
	servtree.h		\
	sourceinfo.h		\
	stdinc.h		\
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

include ../buildsys.mk
