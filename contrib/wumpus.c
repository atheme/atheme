/*
 * Wumpus - 0.1.0
 * Copyright (c) 2006 William Pitcock <nenolod -at- nenolod.net>
 *
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Hunt the Wumpus game implementation.
 *
 * $Id$
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"contrib/wumpus", FALSE, _modinit, _moddeinit,
	"$Id$",
	"William Pitcock <nenolod -at- nenolod.net>"
);

/* contents */
enum {
	E_NOTHING = 0,
	E_WUMPUS,
	E_PIT,
	E_BATS
};

/* room_t: Describes a room that the wumpus or players could be in. */
struct room_ {
	int id;			/* room 3 or whatever */
	list_t exits;		/* old int count == exits.count */
	int contents;
	list_t players;		/* player_t players */
};

typedef struct room_ room_t;

/* player_t: A player object. */
struct player_ {
	user_t    *u;
	room_t    *location;
	int        arrows;
	boolean_t  has_moved;
};

typedef struct player_ player_t;

struct game_ {
	int wumpus;
	int mazesize;
	list_t players;
	boolean_t running;
	boolean_t starting;

	room_t *rmemctx;	/* memory page context */
	service_t *svs;
	list_t cmdtree;
};

typedef struct game_ game_t;

game_t wumpus;

struct __wumpusconfig
{
	char *chan;
	char *nick;
	char *user;
	char *host;
	char *real;
} wumpus_cfg = {
	"#wumpus",
	"Wumpus",
	"wumpus",
	"services.int",
	"Hunt the Wumpus"
};

/* ------------------------------ utility functions */

/* returns 1 or 2 depending on if the wumpus is 1 or 2 rooms away */
static int
distance_to_wumpus(player_t *player)
{
	int i;
	node_t *n, *tn;
	
	LIST_FOREACH(n, player->location->exits.head)
	{
		room_t *r = (room_t *) n->data;

		if (r->contents == E_WUMPUS)
			return 1;

		LIST_FOREACH(tn, r->exits.head)
		{
			room_t *r2 = (room_t *) tn->data;

			if (r2->contents == E_WUMPUS)
				return 2;

			/* we don't evaluate exitpoints at this depth */
		}
	}

	return 0;
}

/* can we move or perform an action on this room? */
static boolean_t
adjacent_room(player_t *p, int id)
{
	node_t *n;

	LIST_FOREACH(n, p->location->exits.head)
	{
		room_t *r = (room_t *) n->data;

		if (r->id == id)
			return TRUE;
	}

	return FALSE;
}

/* finds a player in the list */
static player_t *
find_player(user_t *u)
{
	node_t *n;

	LIST_FOREACH(n, wumpus.players.head)
	{
		player_t *p = n->data;

		if (p->u == u)
			return p;
	}

	return NULL;
}

/* adds a player to the game */
static player_t *
create_player(user_t *u)
{
	player_t *p;

	if (find_player(u))
	{
		notice(wumpus_cfg.nick, u->nick, "You are already playing the game!");
		return;
	}

	if (wumpus.running)
	{
		notice(wumpus_cfg.nick, u->nick, "The game is already in progress. Sorry!");
		return;
	}

	p = smalloc(sizeof(player_t));
	memset(p, '\0', sizeof(player_t));

	p->u = u;
	p->arrows = 5;

	node_add(p, node_create(), &wumpus.players);
}

/* destroys a player object and removes them from the game */
void
resign_player(player_t *player)
{
	node_t *n;

	if (player == NULL)
		return;

	n = node_find(player, &player->location->players);
	node_del(n, &player->location->players);
	node_free(n);

	n = node_find(player, &wumpus.players);
	node_del(n, &wumpus.players);
	node_free(n);

	free(player);
}

/* ------------------------------ game functions */

/* builds the maze, and returns FALSE if the maze is too small */
boolean_t
build_maze(int size)
{
	int i, j;
	room_t *w;

	if (size < 10)
		return FALSE;

	slog(LG_DEBUG, "wumpus: building maze of %d chambers", size);

	/* allocate rooms */
	wumpus.mazesize = size;
	wumpus.rmemctx = scalloc(size, sizeof(room_t));

	for (i = 0; i < size; i++)
	{
		room_t *r = &wumpus.rmemctx[i];
		memset(r, '\0', sizeof(room_t));

		r->id = i;

		/* rooms have 3 exit points, exits are one-way */
		for (j = 0; j < 3; j++)
		{
			int t = rand() % size;
			slog(LG_DEBUG, "wumpus: creating link for route %d -> %d", i, t);
			node_add(&wumpus.rmemctx[t], node_create(), &r->exits);
		}

		slog(LG_DEBUG, "wumpus: finished creating exit paths for chamber %d", i);
	}

	/* place the wumpus in the maze */
	wumpus.wumpus = rand() % size;
	w = &wumpus.rmemctx[wumpus.wumpus];
	w->contents = E_WUMPUS;

	/* pits */
	for (i = 0; i < 5; i++)
	{
		for (j = 0; j < size; j++)
		{
			/* 42 will do very nicely */
			if (rand() % 42 == 0)
			{
				room_t *r = &wumpus.rmemctx[j];

				r->contents = E_PIT;

				slog(LG_DEBUG, "wumpus: added pit to chamber %d", j);
			}
		}
	}

	/* bats */
	for (i = 0; i < 5; i++)
	{
		for (j = 0; j < size; j++)
		{
			/* 42 will do very nicely */
			if (rand() % 42 == 0)
			{
				room_t *r = &wumpus.rmemctx[j];

				r->contents = E_BATS;

				slog(LG_DEBUG, "wumpus: added bats to chamber %d", j);
			}
		}
	}

	slog(LG_DEBUG, "wumpus: built maze");

	return TRUE;
}

/* init_game depends on these */
void move_wumpus(void *unused);
void look_player(player_t *p);

/* sets the game up */
void
init_game(void)
{
	node_t *n;

	build_maze(rand() % 1000);

	/* place players in random positions */
	LIST_FOREACH(n, wumpus.players.head)
	{
		player_t *p = (player_t *) n->data;

		p->location = &wumpus.rmemctx[rand() % wumpus.mazesize];
		node_add(p, node_create(), &p->location->players);

		look_player(p);
	}

	/* timer initialization */
	event_add("move_wumpus", move_wumpus, NULL, 60);

	msg(wumpus_cfg.nick, wumpus_cfg.chan, "The game has started!");

	wumpus.running = TRUE;
}

/* starts the game */
void
start_game(void *unused)
{
	if (wumpus.players.count < 2)
	{
		msg(wumpus_cfg.nick, wumpus_cfg.chan, "Not enough players to play. :(");
		wumpus.starting = FALSE;
		return;
	}

	init_game();
}

/* destroys game objects */
void
end_game(void)
{
	node_t *n, *tn;
	int i;

	/* destroy players */
	LIST_FOREACH_SAFE(n, tn, wumpus.players.head)
		resign_player((player_t *) n->data);

	/* destroy links between rooms */
	for (i = 0; i < wumpus.mazesize; i++)
	{
		room_t *r = &wumpus.rmemctx[i];

		LIST_FOREACH_SAFE(n, tn, r->exits.head)
			node_del(n, &r->exits);
	}

	/* free memory vector */
	free(wumpus.rmemctx);

	wumpus.wumpus = -1;
	wumpus.running = FALSE;

	event_delete(move_wumpus, NULL);

	/* game is now ended */
}

/* gives the player information about their surroundings */
void
look_player(player_t *p)
{
	node_t *n, *tn;

	notice(wumpus_cfg.nick, p->u->nick, "You are in room %d.", p->location->id);

	LIST_FOREACH(n, p->location->exits.head)
	{
		room_t *r = (room_t *) n->data;

		notice(wumpus_cfg.nick, p->u->nick, "You can move to room %d.", r->id);
	}

	/* provide warnings */
	LIST_FOREACH(n, p->location->exits.head)
	{
		room_t *r = (room_t *) n->data;

		LIST_FOREACH(tn, r->exits.head)
		{
			room_t *tr = (room_t *) tn->data;

			if (tr->contents == E_WUMPUS)
				notice(wumpus_cfg.nick, p->u->nick, "You smell a wumpus!");
		}

		if (r->contents == E_WUMPUS)
			notice(wumpus_cfg.nick, p->u->nick, "You smell a wumpus!");
		if (r->contents == E_PIT)
			notice(wumpus_cfg.nick, p->u->nick, "You feel a draft!");
		if (r->contents == E_BATS)
			notice(wumpus_cfg.nick, p->u->nick, "You hear bats!");
		if (r->players.count > 0)
			notice(wumpus_cfg.nick, p->u->nick, "You smell humans!");
	}
}

/* shoot and kill other players */
void
shoot_player(player_t *p, int target_id)
{
	room_t *r;
	player_t *tp;

	if (!p->arrows)
	{
		notice(wumpus_cfg.nick, p->u->nick, "You have no arrows!");
		return;
	}

	if (adjacent_room(p, target_id) == FALSE)
	{
		notice(wumpus_cfg.nick, p->u->nick, "You can't shoot into room %d from here.");
		return;
	}

	if (p->location->id == target_id)
	{
		notice(wumpus_cfg.nick, p->u->nick, "You can only shoot into adjacent rooms!");
		return;
	}

	r = &wumpus.rmemctx[target_id];
	tp = r->players.head ? r->players.head->data : NULL;

	p->arrows--;

	if (!tp)
	{
		notice(wumpus_cfg.nick, p->u->nick, "You shoot at nothing.");
		return;
	}

	/* 50 percent chance of success */
	if (rand() % 2 == 0)
	{
		msg(wumpus_cfg.nick, wumpus_cfg.chan, "\2%s\2 has been killed by \2%s\2!",
			tp->u->nick, p->u->nick);
		resign_player(p);
	}
	else
	{
		notice(wumpus_cfg.nick, tp->u->nick, "You have been shot at from room %d.",
			p->location->id);
		notice(wumpus_cfg.nick, p->u->nick, "You miss what you were shooting at.");
	}
}

/* move the wumpus, the wumpus moves every 60 seconds */
void
move_wumpus(void *unused)
{
	node_t *n, *tn;
	room_t *r;
	int w_kills = 0;
	boolean_t moved = FALSE;

	msg(wumpus_cfg.nick, wumpus_cfg.chan, "You hear footsteps...");

	/* start moving */
	r = &wumpus.rmemctx[wumpus.wumpus]; /* memslice describing the wumpus's current location */
	r->contents = E_NOTHING;

	while (!moved)
	{
		LIST_FOREACH(n, r->exits.head)
		{
			room_t *tr = (room_t *) n->data;

			if (rand() % 42 == 0 && moved == FALSE)
			{
#ifdef DEBUG_AI
				msg(wumpus_cfg.nick, wumpus_cfg.chan, "I moved to chamber %d", tr->id);
#endif

				slog(LG_DEBUG, "wumpus: the wumpus is now in room %d! (was in %d)",
					tr->id, wumpus.wumpus);
				wumpus.wumpus = tr->id;
				tr->contents = E_WUMPUS;

				moved = TRUE;
			}
		}
	}

#ifdef DEBUG_AI
	msg(wumpus_cfg.nick, wumpus_cfg.chan, "On my next turn, I can move to:");
	r = &wumpus.rmemctx[wumpus.wumpus];

	LIST_FOREACH(n, r->exits.head)
	{
		room_t *tr = (room_t *) n->data;

		msg(wumpus_cfg.nick, wumpus_cfg.chan, "- %d", tr->id);
	}
#endif

	LIST_FOREACH_SAFE(n, tn, wumpus.players.head)
	{
		player_t *p = (player_t *) n->data;

		if (wumpus.wumpus == p->location->id)
		{
			notice(wumpus_cfg.nick, p->u->nick, "The wumpus has joined your room and eaten you. Sorry.");
			w_kills++;

			/* player_t *p has been eaten and is no longer in the game */
			resign_player(p);
		}

		if (w_kills)
			msg(wumpus_cfg.nick, wumpus_cfg.chan, "You hear the screams of %d surprised adventurer%s.", w_kills,
				w_kills != 1 ? "s" : "");

		/* prepare for the next turn */
		p->has_moved = FALSE;
	}

	if (wumpus.players.count == 1)
	{
		player_t *p = (player_t *) wumpus.players.head->data;

		msg(wumpus_cfg.nick, wumpus_cfg.chan, "%s won the game! Congratulations!", p->u->nick);

		end_game();
	}
}

/* handles movement requests from players */
void
move_player(player_t *p, int id)
{
	node_t *n;

	if (adjacent_room(p, id) == FALSE)
	{
		notice(wumpus_cfg.nick, p->u->nick, "Sorry, you cannot get to room %d from here.", id);
		return;
	}

	/* What about bats? We check for this first because yeah... */
	if (wumpus.rmemctx[id].contents == E_BATS)
	{
		int target_id = rand() % wumpus.mazesize;

		notice(wumpus_cfg.nick, p->u->nick, "Bats have picked you up and taken you to room %d.",
			target_id);
		msg(wumpus_cfg.nick, wumpus_cfg.chan, "You hear a surprised yell.");

		/* move the bats */
		wumpus.rmemctx[id].contents = E_NOTHING;
		wumpus.rmemctx[target_id].contents = E_BATS;

		id = target_id;

		/* and fall through, sucks if you hit the two conditions below :-P */
	}

	/* Is the wumpus in here? */
	if (wumpus.wumpus == id)
	{
		notice(wumpus_cfg.nick, p->u->nick, "You see the wumpus approaching you. You scream for help, but it is too late.");
		msg(wumpus_cfg.nick, wumpus_cfg.chan, "You hear a blood-curdling scream.");

		/* player_t *p has been killed by the wumpus, remove him from the game */
		resign_player(p);
		return;
	}

	/* What about a pit? */
	if (wumpus.rmemctx[id].contents == E_PIT)
	{
		notice(wumpus_cfg.nick, p->u->nick, "You have fallen into a bottomless pit. Sorry.");
		msg(wumpus_cfg.nick, wumpus_cfg.chan, "You hear a faint wail, which gets fainter and fainter.");

		/* player_t *p has fallen down a hole, remove him from the game */
		resign_player(p);
		return;
	}

	/* we recycle the node_t here for speed */
	n = node_find(p, &p->location->players);
	node_del(n, &p->location->players);

	p->location = &wumpus.rmemctx[id];
	node_add(p, n, &p->location->players);

	/* provide player with information, including their new location */
	look_player(p);

	/* tell players about joins. */
	if (p->location->players.count > 1)
	{
		LIST_FOREACH(n, p->location->players.head)
		{
			if (n->data != p)
			{
				player_t *tp = (player_t *) n->data;

				notice(wumpus_cfg.nick, tp->u->nick, "%s has joined room %d with you.",
					p->u->nick, id);
				notice(wumpus_cfg.nick, p->u->nick, "You see %s!",
					tp->u->nick);
			}
		}
	}
}

/* ------------------------------ -*-atheme-*- code */

void cmd_start(char *origin)
{
	if (wumpus.running || wumpus.starting)
	{
		notice(wumpus_cfg.nick, origin, "A game is already in progress. Sorry.");
		return;
	}

	msg(wumpus_cfg.nick, wumpus_cfg.chan, "\2%s\2 has started the game! Use \2/msg Wumpus JOIN\2 to play! You have\2 60 seconds\2.",
		origin);

	wumpus.starting = TRUE;

	event_add_once("start_game", start_game, NULL, 60);
}

/* reference tuple for the above code: cmd_start */
command_t wumpus_start = { "START", "Starts the game.", AC_NONE, cmd_start };

void cmd_join(char *origin)
{
	player_t *p;

	if (!wumpus.starting || wumpus.running)
	{
		notice(wumpus_cfg.nick, origin, "You cannot use this command right now. Sorry.");
		return;
	}

	p = create_player(user_find(origin));

	msg(wumpus_cfg.nick, wumpus_cfg.chan, "\2%s\2 has joined the game!", origin);
}

command_t wumpus_join = { "JOIN", "Joins the game.", AC_NONE, cmd_join };

void cmd_move(char *origin)
{
	player_t *p = find_player(user_find(origin));
	char *id = strtok(NULL, " ");

	if (!p)
	{
		notice(wumpus_cfg.nick, origin, "You must be playing the game in order to use this command.");
		return;
	}

	if (!id)
	{
		notice(wumpus_cfg.nick, origin, "You must provide a room to move to.");
		return;
	}

	if (!wumpus.running)
	{
		notice(wumpus_cfg.nick, origin, "The game must be running in order to use this command.");
		return;
	}

	move_player(p, atoi(id));
}

command_t wumpus_move = { "MOVE", "Move to another room.", AC_NONE, cmd_move };

void cmd_shoot(char *origin)
{
	player_t *p = find_player(user_find(origin));
	char *id = strtok(NULL, " ");

	if (!p)
	{
		notice(wumpus_cfg.nick, origin, "You must be playing the game in order to use this command.");
		return;
	}

	if (!id)
	{
		notice(wumpus_cfg.nick, origin, "You must provide a room to shoot at.");
		return;
	}

	if (!wumpus.running)
	{
		notice(wumpus_cfg.nick, origin, "The game must be running in order to use this command.");
		return;
	}

	shoot_player(p, atoi(id));
}

command_t wumpus_shoot = { "SHOOT", "Shoot at another room.", AC_NONE, cmd_shoot };

void cmd_resign(char *origin)
{
	player_t *p = find_player(user_find(origin));

	if (!p)
	{
		notice(wumpus_cfg.nick, origin, "You must be playing the game in order to use this command.");
		return;
	}

	if (!wumpus.running)
	{
		notice(wumpus_cfg.nick, origin, "The game must be running in order to use this command.");
		return;
	}

	msg(wumpus_cfg.nick, wumpus_cfg.chan, "\2%s\2 has quit the game!", p->u->nick);

	resign_player(p);
}

command_t wumpus_resign = { "RESIGN", "Resign from the game.", AC_NONE, cmd_resign };

void cmd_reset(char *origin)
{
	if (wumpus.running)
	{
		msg(wumpus_cfg.nick, wumpus_cfg.chan, "\2%s\2 has ended the game.", origin);

		end_game();

		wumpus.running = FALSE;
		wumpus.starting = FALSE;
	}
}

command_t wumpus_reset = { "RESET", "Resets the game.", AC_IRCOP, cmd_reset };

void cmd_help(char *origin)
{
	command_help(wumpus.svs->name, origin, &wumpus.cmdtree);
}

command_t wumpus_help = { "HELP", "Displays this command listing.", AC_NONE, cmd_help };

/* removes quitting players */
void
user_deleted(void *vptr)
{
	user_t *u = (user_t *) vptr;
	player_t *p;

	if ((p = find_player(u)) != NULL)
	{
		msg(wumpus_cfg.nick, wumpus_cfg.chan, "\2%s\2 has quit the game!", p->u->nick);
		resign_player(p);
	}
}

void
_handler(char *origin, uint8_t parc, char *parv[])
{
        char *cmd;
        char orig[BUFSIZE];

        /* this should never happen */
        if (parv[0][0] == '&')
        {
                slog(LG_ERROR, "services(): got parv with local channel: %s", parv[0]);
                return;
        }

        /* make a copy of the original for debugging */
        strlcpy(orig, parv[parc - 1], BUFSIZE);

        /* lets go through this to get the command */
        cmd = strtok(parv[parc - 1], " ");

        if (!cmd)
                return;

        /* take the command through the hash table */
        command_exec(wumpus.svs, origin, cmd, &wumpus.cmdtree);
}

void
burst_the_wumpus(void *unused)
{
	if (!wumpus.svs)
		wumpus.svs = add_service(wumpus_cfg.nick, wumpus_cfg.user, wumpus_cfg.host, wumpus_cfg.real, _handler);
	
	join(wumpus_cfg.chan, wumpus_cfg.nick);	/* what if we're not ready? then this is a NOOP */
}

/* start handler */
void
_modinit(module_t *m)
{
	if (cold_start)
	{
		hook_add_event("config_ready");
		hook_add_hook("config_ready", burst_the_wumpus);
	}
	else
		burst_the_wumpus(NULL);

	hook_add_event("user_delete");
	hook_add_hook("user_delete", user_deleted);

	command_add(&wumpus_help, &wumpus.cmdtree);
	command_add(&wumpus_start, &wumpus.cmdtree);
	command_add(&wumpus_join, &wumpus.cmdtree);
	command_add(&wumpus_move, &wumpus.cmdtree);
	command_add(&wumpus_shoot, &wumpus.cmdtree);
	command_add(&wumpus_resign, &wumpus.cmdtree);
	command_add(&wumpus_reset, &wumpus.cmdtree);
}

void
_moddeinit(void)
{
	/* cleanup after ourselves if necessary */
	if (wumpus.running)
		end_game();

	del_service(wumpus.svs);

	hook_del_hook("user_delete", user_deleted);

	command_delete(&wumpus_help, &wumpus.cmdtree);
	command_delete(&wumpus_start, &wumpus.cmdtree);
	command_delete(&wumpus_join, &wumpus.cmdtree);
	command_delete(&wumpus_move, &wumpus.cmdtree);
	command_delete(&wumpus_shoot, &wumpus.cmdtree);
	command_delete(&wumpus_resign, &wumpus.cmdtree);
	command_delete(&wumpus_reset, &wumpus.cmdtree);

	event_delete(move_wumpus, NULL);
}
