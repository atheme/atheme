/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Protocol module stuff.
 *
 * $Id: pmodule.h 5724 2006-07-04 16:06:20Z w00t $
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
E int authservice_loaded;

/*  -- what the HELL are these used for? A grep reveals nothing.. --w00t
 *  -- they are used to provide a hint to third-party module coders about what
 *     ircd they are working with. --nenolod
 */
#define PROTOCOL_ASUKA			1
#define PROTOCOL_BAHAMUT		2
#define PROTOCOL_CHARYBDIS		3
#define PROTOCOL_DREAMFORGE		4
#define PROTOCOL_HYPERION		5
#define PROTOCOL_INSPIRCD		6
#define PROTOCOL_IRCNET			7
#define PROTOCOL_MONKEY			8 /* obsolete */
#define PROTOCOL_PLEXUS			9
#define PROTOCOL_PTLINK			10
#define PROTOCOL_RATBOX			11
#define PROTOCOL_SCYLLA			12
#define PROTOCOL_SHADOWIRCD		13
#define PROTOCOL_SORCERY		14
#define PROTOCOL_ULTIMATE3		15
#define PROTOCOL_UNDERNET		16
#define PROTOCOL_UNREAL			17
#define PROTOCOL_SOLIDIRCD		18
#define PROTOCOL_NEFARIOUS		19

#define PROTOCOL_OTHER			255

#endif
