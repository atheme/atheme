SHARED_LIB	= ${LIB_PREFIX}athemecore${LIB_SUFFIX}
LIB_MAJOR	= 1
LIB_MINOR	= 0
HELP_LINGUAS	= es ru fr

SRCS =				\
	account.c		\
	arc4random.c		\
	atheme.c		\
	auth.c			\
	authcookie.c		\
	base64.c		\
	channels.c		\
	cidr.c			\
	cmode.c			\
	commandtree.c		\
	conf.c			\
	confprocess.c		\
	connection.c		\
	crypto.c		\
	ctcp-common.c		\
	culture.c		\
	database_backend.c	\
	datastream.c		\
	digest_be_md5.c		\
	digest_fe_internal.c	\
	digest_fe_internal_testsuite.c	\
	entity.c		\
	$(EXPLICIT_BZERO_C)	\
	flags.c			\
	function.c		\
	help.c			\
	hook.c			\
	linker.c		\
	logger.c		\
	match.c			\
	memory.c		\
	module.c		\
	node.c			\
	object.c		\
	packet.c		\
	phandler.c		\
	pmodule.c		\
	privs.c			\
	ptasks.c		\
	$(QRCODE_C)		\
	res.c			\
	reslib.c		\
	send.c			\
	servers.c		\
	services.c		\
	servtree.c		\
	sharedheap.c		\
	signal.c		\
	snprintf.c		\
	string.c		\
	strshare.c		\
	svsignore.c		\
	table.c			\
	template.c		\
	tokenize.c		\
	ubase64.c		\
	uid.c			\
	uplink.c		\
	users.c			\
	version.c

include ../extra.mk
include ../buildsys.mk

CPPFLAGS	+= $(PCRE_CFLAGS) $(LIBQRENCODE_CFLAGS) -I../include -I.
CFLAGS		+= $(LIB_CFLAGS)
LIBS		+= $(LIBINTL) $(PCRE_LIBS) $(LIBQRENCODE_LIBS)

build: depend all
