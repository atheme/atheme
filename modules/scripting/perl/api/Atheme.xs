#include "atheme_perl.h"

typedef sourceinfo_t *Atheme_Sourceinfo;
typedef perl_command_t *Atheme_Command;
typedef service_t *Atheme_Service;
typedef user_t *Atheme_User;
typedef object_t *Atheme_Object;
typedef object_t *Atheme_Object_MetadataHash;
typedef myentity_t *Atheme_Entity;
typedef myuser_t *Atheme_Account;
typedef channel_t *Atheme_Channel;
typedef chanuser_t *Atheme_ChanUser;
typedef mychan_t *Atheme_ChannelRegistration;
typedef chanacs_t *Atheme_ChanAcs;
typedef mynick_t *Atheme_NickRegistration;
typedef server_t *Atheme_Server;

typedef perl_list_t *Atheme_Internal_List;


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
