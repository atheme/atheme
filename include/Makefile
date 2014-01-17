include ../extra.mk

.PHONY: serno.h

SUBDIRS = inline protocol

DISTCLEAN = hooktypes.h serno.h
pre-depend: ${DISTCLEAN}

INCLUDES = \
	abirev.h		\
	account.h		\
	atheme.h		\
	atheme_memory.h		\
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
	md5.h			\
	module.h		\
	object.h		\
	phandler.h		\
	pmodule.h		\
	privs.h			\
	res.h			\
	reslib.h		\
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
	$(info Generate: $@)
	@revh=`git log -1 --pretty=oneline | cut -d' ' -f1 2>/dev/null` || :; \
	[ -z "$$revh" ] || echo "#define SERNO \"$$revh\"" >serno.h

include ../buildsys.mk
