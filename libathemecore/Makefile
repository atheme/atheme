SHARED_LIB	= ${LIB_PREFIX}athemecore${LIB_SUFFIX}
LIB_MAJOR	= 1
LIB_MINOR	= 0
HELP_LINGUAS	= es ru fr

SRCS =					\
	account.c			\
	$(ARC4RANDOM_COND_C)		\
	atheme.c			\
	auth.c				\
	authcookie.c			\
	base64.c			\
	channels.c			\
	cidr.c				\
	cmode.c				\
	commandhelp.c			\
	commandtree.c			\
	conf.c				\
	confprocess.c			\
	connection.c			\
	crypto.c			\
	ctcp-common.c			\
	culture.c			\
	database_backend.c		\
	datastream.c			\
	$(DIGEST_FRONTEND_COND_C)	\
	digest_fe_shared.c		\
	digest_testsuite.c		\
	entity.c			\
	flags.c				\
	function.c			\
	hook.c				\
	linker.c			\
	logger.c			\
	match.c				\
	memory.c			\
	module.c			\
	node.c				\
	object.c			\
	packet.c			\
	phandler.c			\
	pmodule.c			\
	privs.c				\
	ptasks.c			\
	$(QRCODE_COND_C)		\
	send.c				\
	servers.c			\
	services.c			\
	servtree.c			\
	sharedheap.c			\
	signal.c			\
	snprintf.c			\
	string.c			\
	strshare.c			\
	svsignore.c			\
	table.c				\
	template.c			\
	tokenize.c			\
	ubase64.c			\
	uid.c				\
	uplink.c			\
	users.c				\
	version.c

include ../extra.mk
include ../buildsys.mk

CPPFLAGS	+= $(PCRE_CFLAGS) $(LIBQRENCODE_CFLAGS) -I../include -I.
CFLAGS		+= $(LIB_CFLAGS)
LIBS		+= $(DIGEST_API_LIBS) $(LIBINTL) $(PCRE_LIBS) $(LIBQRENCODE_LIBS) $(LIBSODIUM_LIBS)

build: depend all
