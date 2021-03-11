include ../extra.mk

.PHONY: serno.h

SUBDIRS = inline protocol

DISTCLEAN = hooktypes.h instpaths.h serno.h
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
	instpaths.h		\
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

instpaths.h: ../extra.mk instpaths.h.in
	$(info Generate: $@)
	@$(SED)                                         \
		-e 's|@BINDIR[@]|$(bindir)|g'           \
		-e 's|@DATADIR[@]|$(DATADIR)|g'         \
		-e 's|@DOCDIR[@]|$(DOCDIR)|g'           \
		-e 's|@LOCALEDIR[@]|$(LOCALEDIR)|g'     \
		-e 's|@LOGDIR[@]|$(LOGDIR)|g'           \
		-e 's|@MODDIR[@]|$(MODDIR)|g'           \
		-e 's|@PREFIX[@]|$(prefix)|g'           \
		-e 's|@RUNDIR[@]|$(RUNDIR)|g'           \
		-e 's|@SHAREDIR[@]|$(SHAREDIR)|g'       \
		-e 's|@SYSCONFDIR[@]|$(sysconfdir)|g'   \
	< "$@.in" > "$@"

serno.h:
	@sh mkserno.sh

include ../buildsys.mk
