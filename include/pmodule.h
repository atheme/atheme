/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Protocol module stuff.
 *
 * $Id: pmodule.h 2835 2005-10-11 05:27:26Z terminal $
 */

#ifndef PMODULE_H
#define PMODULE_H

typedef struct pcommand_ pcommand_t;

struct pcommand_ {
	char	*token;
	void	(*handler)(char *origin, uint8_t parc, char *parv[]);
};

E list_t pcommands[HASHSIZE];

E void pcommand_init(void);
E void pcommand_add(char *token,
        void (*handler)(char *origin, uint8_t parc, char *parv[]));
E void pcommand_delete(char *token);
E pcommand_t *pcommand_find(char *token);

E boolean_t pmodule_loaded;
E boolean_t backend_loaded;

#define PROTOCOL_ASUKA			1
#define PROTOCOL_BAHAMUT		2
#define PROTOCOL_CHARYBDIS		3
#define PROTOCOL_DREAMFORGE		4
#define PROTOCOL_HYPERION		5
#define PROTOCOL_INSPIRCD		6 /* XXX to be removed soon */
#define PROTOCOL_IRCNET			7
#define PROTOCOL_MONKEY			8
#define PROTOCOL_PLEXUS			9
#define PROTOCOL_PTLINK			10
#define PROTOCOL_RATBOX			11
#define PROTOCOL_SCYLLA			12
#define PROTOCOL_SHADOWIRCD		13
#define PROTOCOL_SORCERY		14
#define PROTOCOL_ULTIMATE3		15
#define PROTOCOL_UNDERNET		16
#define PROTOCOL_UNREAL			17

#define PROTOCOL_OTHER			255

#endif
