#include "atheme_perl.h"

typedef struct sourceinfo *Atheme_Sourceinfo;
typedef struct perl_command *Atheme_Command;
typedef struct service *Atheme_Service;
typedef struct user *Atheme_User;
typedef struct atheme_object *Atheme_Object;
typedef struct atheme_object *Atheme_Object_MetadataHash;
typedef struct myentity *Atheme_Entity;
typedef struct myuser *Atheme_Account;
typedef struct channel *Atheme_Channel;
typedef struct chanuser *Atheme_ChanUser;
typedef struct mychan *Atheme_ChannelRegistration;
typedef struct chanacs *Atheme_ChanAcs;
typedef struct mynick *Atheme_NickRegistration;
typedef struct server *Atheme_Server;

typedef struct perl_list *Atheme_Internal_List;


MODULE = Atheme			PACKAGE = Atheme

INCLUDE: services.xs
INCLUDE: sourceinfo.xs
INCLUDE: commands.xs
INCLUDE: user.xs
INCLUDE: object.xs
INCLUDE: metadata.xs
INCLUDE: account.xs
INCLUDE: channel.xs
INCLUDE: channelregistration.xs
INCLUDE: config.xs
INCLUDE: nickregistration.xs
INCLUDE: server.xs

INCLUDE: internal_list.xs

INCLUDE: hooks.xs
INCLUDE: log.xs

MODULE = Atheme			PACKAGE = Atheme

void
wallops (const char * message)
CODE:
    wallops ("%s", message);
