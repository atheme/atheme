SHARED_LIB		= ${LIB_PREFIX}athemecore${LIB_SUFFIX}
LIB_MAJOR	= 1
LIB_MINOR	= 0
HELP_LINGUAS	= es ru fr

BASE_SRCS =				\
	account.c		\
	atheme.c		\
	arc4random.c		\
	auth.c		\
	authcookie.c		\
	base64.c		\
	channels.c		\
	cidr.c		\
	cmode.c		\
	commandtree.c		\
	ctcp-common.c		\
	conf.c		\
	confprocess.c		\
	connection.c		\
	crypto.c		\
	culture.c		\
	database_backend.c	\
	datastream.c		\
	entity.c	\
	flags.c		\
	function.c		\
	help.c		\
	hook.c		\
	linker.c		\
	logger.c		\
	match.c		\
	md5.c			\
	memory.c		\
	module.c		\
	node.c		\
	object.c		\
	packet.c		\
	phandler.c		\
	pmodule.c		\
	privs.c		\
	ptasks.c		\
	res.c		\
	reslib.c	\
	qrcode.c	\
	send.c		\
	servers.c		\
	services.c		\
	servtree.c		\
	sharedheap.c		\
	signal.c		\
	snprintf.c		\
	string.c		\
	strshare.c		\
	svsignore.c		\
	table.c		\
	template.c		\
	tokenize.c		\
	ubase64.c		\
	users.c		\
	uid.c			\
	uplink.c

SRCS = ${BASE_SRCS} version.c

include ../buildsys.mk
include ../extra.mk

CPPFLAGS	+= $(MOWGLI_CFLAGS) $(PCRE_CFLAGS) $(LIBQRENCODE_CFLAGS) -I../include -DBINDIR=\"$(bindir)\"
CFLAGS		+= $(LIB_CFLAGS)
LIBS		+= $(MOWGLI_LIBS) $(PCRE_LIBS) $(LIBINTL) $(LIBQRENCODE_LIBS)
LDFLAGS         += $(LDFLAGS_RPATH)

build: depend all
