/*
 * Wumpus - 0.1.0
 * Copyright (c) 2006 William Pitcock <nenolod -at- nenolod.net>
 * Portions copyright (c) 2006 Kiyoshi Aman <kiyoshi.aman -at- gmail.com>
 *
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Hunt the Wumpus game implementation.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"contrib/wumpus", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"William Pitcock <nenolod -at- nenolod.net>"
);

/* contents */
enum {
	E_NOTHING = 0,
	E_WUMPUS,
	E_PIT,
	E_BATS,
	E_ARROWS,
	E_CRYSTALBALL
};

/* room_t: Describes a room that the wumpus or players could be in. */
struct room_ {
	int id;			/* room 3 or whatever */
	mowgli_list_t exits;		/* old int count == exits.count */
	int contents;
	mowgli_list_t players;		/* player_t players */
};

typedef struct room_ room_t;

/* player_t: A player object. */
struct player_ {
	user_t    *u;
	room_t    *location;
	int        arrows;
	int	   hp;
	bool  has_moved;
};

typedef struct player_ player_t;

struct game_ {
	int wumpus;
	int mazesize;
	mowgli_list_t players;
	bool running;
	bool starting;

	room_t *rmemctx;	/* memory page context */
	service_t *svs;
	int wump_hp;
	int speed;
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
	mowgli_node_t *n, *tn;
	
	MOWGLI_ITER_FOREACH(n, player->location->exits.head)
	{
		room_t *r = (room_t *) n->data;

		if (r->contents == E_WUMPUS)
			return 1;

		MOWGLI_ITER_FOREACH(tn, r->exits.head)
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
static bool
adjacent_room(player_t *p, int id)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, p->location->exits.head)
	{
		room_t *r = (room_t *) n->data;

		if (r->id == id)
			return true;
	}

	return false;
}

/* finds a player in the list */
static player_t *
find_player(user_t *u)
{
	mowgli_node_t *n;

	MOWGLI_ITER_FOREACH(n, wumpus.players.head)
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
		return NULL;
	}

	if (wumpus.running)
	{
		notice(wumpus_cfg.nick, u->nick, "The game is already in progress. Sorry!");
		return NULL;
	}

	p = smalloc(sizeof(player_t));
	memset(p, '\0', sizeof(player_t));

	p->u = u;
	p->arrows = 10;
	p->hp = 30;

	mowgli_node_add(p, mowgli_node_create(), &wumpus.players);

	return p;
}

/* destroys a player object and removes them from the game */
static void
resign_player(player_t *player)
{
	mowgli_node_t *n;

	if (player == NULL)
		return;
	
	if (player->location)
	{
		n = mowgli_node_find(player, &player->location->players);
		mowgli_node_delete(n, &player->location->players);
		mowgli_node_free(n);
	}

	n = mowgli_node_find(player, &wumpus.players);
	mowgli_node_delete(n, &wumpus.players);
	mowgli_node_free(n);

	free(player);
}

/* ------------------------------ game functions */

/* builds the maze, and returns false if the maze is too small */
static bool
build_maze(int size)
{
	int i, j;
	room_t *w;

	if (size < 10)
		return false;

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
		for (j = 0; j < 3 && r->exits.count < 3; j++)
		{
			int t = rand() % size;

			/* make sure this isn't a tunnel to itself */
			while (t == r->id)
			{
				mowgli_node_t *rn;
				t = rand() % size;

				/* also check that this path doesn't already exist. */
				MOWGLI_ITER_FOREACH(rn, r->exits.head)
				{
					room_t *rm = (room_t *) rn->data;

					if (rm->id == t)
						t = r->id;
				}
			}

			slog(LG_DEBUG, "wumpus: creating link for route %d -> %d", i, t);
			mowgli_node_add(&wumpus.rmemctx[t], mowgli_node_create(), &r->exits);
		}

		slog(LG_DEBUG, "wumpus: finished creating exit paths for chamber %d", i);
	}

	/* place the wumpus in the maze */
	wumpus.wumpus = rand() % size;
	w = &wumpus.rmemctx[wumpus.wumpus];
	w->contents = E_WUMPUS;

	/* pits */
	for (j = 0; j < size; j++)
	{
		/* 42 will do very nicely */
		if (rand() % (42 * 2) == 0)
		{
			room_t *r = &wumpus.rmemctx[j];

			r->contents = E_PIT;

			slog(LG_DEBUG, "wumpus: added pit to chamber %d", j);
		}
	}

	/* bats */
	for (i = 0; i < 2; i++)
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

	/* arrows */
	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < size; j++)
		{
			/* 42 will do very nicely */
			if (rand() % 42 == 0)
			{
				room_t *r = &wumpus.rmemctx[j];

				r->contents = E_ARROWS;

				slog(LG_DEBUG, "wumpus: added arrows to chamber %d", j);
			}
		}
	}

	/* find a place to put the crystal ball */
	w = &wumpus.rmemctx[rand() % size];
	w->contents = E_CRYSTALBALL;
	slog(LG_DEBUG, "wumpus: added crystal ball to chamber %d", w->id);

	/* ok, do some sanity checking */
	for (j = 0; j < size; j++)
		if (wumpus.rmemctx[j].exits.count < 3)
		{
			slog(LG_DEBUG, "wumpus: sanity checking failed");
			return false;
		}

	slog(LG_DEBUG, "wumpus: built maze");

	return true;
}

/* init_game depends on these */
void move_wumpus(void *unused);
void look_player(player_t *p);
void end_game(void);

/* sets the game up */
static void
init_game(void)
{
	mowgli_node_t *n;

	if (!build_maze(rand() % 100))
	{
		msg(wumpus_cfg.nick, wumpus_cfg.chan, "Maze generation failed, please try again.");
		end_game();
		return;
	}

	/* place players in random positions */
	MOWGLI_ITER_FOREACH(n, wumpus.players.head)
	{
		player_t *p = (player_t *) n->data;

		p->location = &wumpus.rmemctx[rand() % wumpus.mazesize];
		mowgli_node_add(p, mowgli_node_create(), &p->location->players);

		look_player(p);
	}

	/* timer initialization */
	event_add("move_wumpus", move_wumpus, NULL, 60);

	msg(wumpus_cfg.nick, wumpus_cfg.chan, "The game has started!");

	wumpus.running = true;
	wumpus.speed = 60;
	wumpus.wump_hp = 70;
}

/* starts the game */
static void
start_game(void *unused)
{
	wumpus.starting = false;

	if (wumpus.players.count < 2)
	{
		msg(wumpus_cfg.nick, wumpus_cfg.chan, "Not enough players to play. :(");
		return;
	}

	init_game();
}

/* destroys game objects */
void
end_game(void)
{
	mowgli_node_t *n, *tn;
	int i;

	/* destroy players */
	MOWGLI_ITER_FOREACH_SAFE(n, tn, wumpus.players.head)
		resign_player((player_t *) n->data);

	/* free memory vector */
	if (wumpus.rmemctx)
	{
		/* destroy links between rooms */
		for (i = 0; i < wumpus.mazesize; i++)
		{
			room_t *r = &wumpus.rmemctx[i];

			MOWGLI_ITER_FOREACH_SAFE(n, tn, r->exits.head)
				mowgli_node_delete(n, &r->exits);
		}
		free(wumpus.rmemctx);
		wumpus.rmemctx = NULL;
	}

	wumpus.wumpus = -1;
	wumpus.running = false;

	event_delete(move_wumpus, NULL);

	/* game is now ended */
}

/* gives the player information about their surroundings */
void
look_player(player_t *p)
{
	mowgli_node_t *n;

	return_if_fail(p != NULL);
	return_if_fail(p->location != NULL);

	notice(wumpus_cfg.nick, p->u->nick, "You are in room %d.", p->location->id);

	MOWGLI_ITER_FOREACH(n, p->location->exits.head)
	{
		room_t *r = (room_t *) n->data;

		notice(wumpus_cfg.nick, p->u->nick, "You can move to room %d.", r->id);
	}

	if (distance_to_wumpus(p))
		notice(wumpus_cfg.nick, p->u->nick, "You smell a wumpus!");

	/* provide warnings */
	MOWGLI_ITER_FOREACH(n, p->location->exits.head)
	{
		room_t *r = (room_t *) n->data;

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
static void
shoot_player(player_t *p, int target_id)
{
	room_t *r;
	player_t *tp;
	/* chance to hit; moved up here for convenience. */
	int hit = rand() % 3;

	if (!p->arrows)
	{
		notice(wumpus_cfg.nick, p->u->nick, "You have no arrows!");
		return;
	}

	if (adjacent_room(p, target_id) == false)
	{
		notice(wumpus_cfg.nick, p->u->nick, "You can't shoot into room %d from here.", target_id);
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

	if ((!tp) && (r->contents != E_WUMPUS))
	{
		notice(wumpus_cfg.nick, p->u->nick, "You shoot at nothing.");
		return;
	}

	if (tp)
	{
		if ((hit < 2) && (tp->hp <= 10))
		{
			msg(wumpus_cfg.nick, wumpus_cfg.chan, "\2%s\2 has been killed by \2%s\2!",
				tp->u->nick, p->u->nick);
			resign_player(tp);
		}
		else if ((tp->hp > 0) && (hit < 2)) {
			notice(wumpus_cfg.nick, tp->u->nick,
				"You were hit by an arrow from room %d.",p->location->id);
			notice(wumpus_cfg.nick, p->u->nick, "You hit something.");
			tp->hp -= 10;
		}
		else
		{
			notice(wumpus_cfg.nick, tp->u->nick, "You have been shot at from room %d.",
				p->location->id);
			notice(wumpus_cfg.nick, p->u->nick, "You miss what you were shooting at.");
		}
	}
	else if (r->contents == E_WUMPUS) /* Shootin' at the wumpus, we are... */
	{
		if (((wumpus.wump_hp > 0) && wumpus.wump_hp <= 5) && (hit < 2))
			/* we killed the wumpus */
		{
			notice(wumpus_cfg.nick, p->u->nick, "You have killed the wumpus!");
			msg(wumpus_cfg.nick, wumpus_cfg.chan, "The wumpus was killed by \2%s\2.",
				p->u->nick);
			msg(wumpus_cfg.nick, wumpus_cfg.chan,
				"%s has won the game! Congratulations!", p->u->nick);
			end_game();
		}
		else if ((wumpus.wump_hp > 5) && (hit < 2))
		{
			notice(wumpus_cfg.nick, p->u->nick,
				"You shoot the Wumpus, but he shrugs it off and seems angrier!");

			wumpus.wump_hp -= 5;
			wumpus.speed -= 3;

			move_wumpus(NULL);
			event_delete(move_wumpus,NULL);
			event_add("move_wumpus",move_wumpus,NULL,wumpus.speed);
		}
		else
		{
			notice(wumpus_cfg.nick, p->u->nick, "You miss what you were shooting at.");
			move_wumpus(NULL);
		}
	}
}

/* move_wumpus depends on this */
void regen_obj(int);

/* check for last-man-standing win condition. */
static void
check_last_person_alive(void)
{
	if (wumpus.players.count == 1)
	{
		player_t *p = (player_t *) wumpus.players.head->data;

		msg(wumpus_cfg.nick, wumpus_cfg.chan, "%s won the game! Congratulations!", p->u->nick);

		end_game();
	}
	else if (wumpus.players.count == 0)
	{	
		msg(wumpus_cfg.nick, wumpus_cfg.chan, "Everyone lost. Sucks. :(");
		end_game();
	}
}

/* move the wumpus, the wumpus moves every 60 seconds */
void
move_wumpus(void *unused)
{
	mowgli_node_t *n, *tn;
	room_t *r;
	int w_kills = 0;
	bool moved = false;

	/* can we do any of this? if this is null, we really shouldn't be here */
	if (wumpus.rmemctx == NULL)
	{
		slog(LG_DEBUG, "wumpus: move_wumpus() called while game not running!");
		event_delete(move_wumpus, NULL);
		return;
	}

	msg(wumpus_cfg.nick, wumpus_cfg.chan, "You hear footsteps...");

	/* start moving */
	r = &wumpus.rmemctx[wumpus.wumpus]; /* memslice describing the wumpus's current location */

	regen_obj(r->contents);
	r->contents = E_NOTHING;

	while (!moved)
	{
		MOWGLI_ITER_FOREACH(n, r->exits.head)
		{
			room_t *tr = (room_t *) n->data;

			if (rand() % 42 == 0 && moved == false)
			{
#ifdef DEBUG_AI
				msg(wumpus_cfg.nick, wumpus_cfg.chan, "I moved to chamber %d", tr->id);
#endif

				slog(LG_DEBUG, "wumpus: the wumpus is now in room %d! (was in %d)",
					tr->id, wumpus.wumpus);
				wumpus.wumpus = tr->id;
				tr->contents = E_WUMPUS;

				moved = true;
			}
		}
	}

#ifdef DEBUG_AI
	msg(wumpus_cfg.nick, wumpus_cfg.chan, "On my next turn, I can move to:");
	r = &wumpus.rmemctx[wumpus.wumpus];

	MOWGLI_ITER_FOREACH(n, r->exits.head)
	{
		room_t *tr = (room_t *) n->data;

		msg(wumpus_cfg.nick, wumpus_cfg.chan, "- %d", tr->id);
	}
#endif

	MOWGLI_ITER_FOREACH_SAFE(n, tn, wumpus.players.head)
	{
		player_t *p = (player_t *) n->data;

		if (wumpus.wumpus == p->location->id)
		{
			notice(wumpus_cfg.nick, p->u->nick, "The wumpus has joined your room and eaten you. Sorry.");
			w_kills++;

			/* player_t *p has been eaten and is no longer in the game */
			resign_player(p);
		}

		/* prepare for the next turn */
		p->has_moved = false;
	}

	/* report any wumpus kills */
	if (w_kills)
		msg(wumpus_cfg.nick, wumpus_cfg.chan, "You hear the screams of %d surprised adventurer%s.", w_kills,
			w_kills != 1 ? "s" : "");

	check_last_person_alive();
}

/* regenerates objects */
void
regen_obj(int obj)
{
	wumpus.rmemctx[rand() % 42].contents = obj;
}

/* handles movement requests from players */
static void
move_player(player_t *p, int id)
{
	mowgli_node_t *n;

	if (adjacent_room(p, id) == false)
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
		check_last_person_alive();
		return;
	}

	/* What about a pit? */
	if (wumpus.rmemctx[id].contents == E_PIT)
	{
		notice(wumpus_cfg.nick, p->u->nick, "You have fallen into a bottomless pit. Sorry.");
		msg(wumpus_cfg.nick, wumpus_cfg.chan, "You hear a faint wail, which gets fainter and fainter.");

		/* player_t *p has fallen down a hole, remove him from the game */
		resign_player(p);
		check_last_person_alive();
		return;
	}

	/* and arrows? */
	if (wumpus.rmemctx[id].contents == E_ARROWS)
	{
		if (p->arrows == 0)
		{
			notice(wumpus_cfg.nick, p->u->nick, "You found some arrows. You pick them up and continue on your way.");
			p->arrows += 5;
		}
		else
			notice(wumpus_cfg.nick, p->u->nick, "You found some arrows. You don't have any room to take them however, "
						"so you break them in half and continue on your way.");
		
		wumpus.rmemctx[id].contents = E_NOTHING;

		regen_obj(E_ARROWS);
	}

	/* crystal ball */
	if (wumpus.rmemctx[id].contents == E_CRYSTALBALL)
	{
		notice(wumpus_cfg.nick, p->u->nick, "You find a strange pulsating crystal ball. You examine it, and it shows room %d with the wumpus in it.",
			wumpus.wumpus);
		notice(wumpus_cfg.nick, p->u->nick, "The crystal ball then vanishes into the miasma.");

		wumpus.rmemctx[id].contents = E_NOTHING;
		wumpus.rmemctx[rand() % wumpus.mazesize].contents = E_CRYSTALBALL;
	}

	/* we recycle the mowgli_node_t here for speed */
	n = mowgli_node_find(p, &p->location->players);
	mowgli_node_delete(n, &p->location->players);
	mowgli_node_free(n);

	p->location = &wumpus.rmemctx[id];
	mowgli_node_add(p, mowgli_node_create(), &p->location->players);

	/* provide player with information, including their new location */
	look_player(p);

	/* tell players about joins. */
	if (p->location->players.count > 1)
	{
		MOWGLI_ITER_FOREACH(n, p->location->players.head)
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

static void cmd_start(sourceinfo_t *si, int parc, char *parv[])
{
	if (wumpus.running || wumpus.starting)
	{
		notice(wumpus_cfg.nick, si->su->nick, "A game is already in progress. Sorry.");
		return;
	}

	msg(wumpus_cfg.nick, wumpus_cfg.chan, "\2%s\2 has started the game! Use \2/msg Wumpus JOIN\2 to play! You have\2 60 seconds\2.",
		si->su->nick);

	wumpus.starting = true;

	event_add_once("start_game", start_game, NULL, 60);
}

/* reference tuple for the above code: cmd_start */
command_t wumpus_start = { "START", "Starts the game.", AC_NONE, 0, cmd_start, { .path = "" } };

static void cmd_join(sourceinfo_t *si, int parc, char *parv[])
{
	player_t *p;

	if (!wumpus.starting || wumpus.running)
	{
		notice(wumpus_cfg.nick, si->su->nick, "You cannot use this command right now. Sorry.");
		return;
	}

	p = create_player(si->su);

	if (p)
		msg(wumpus_cfg.nick, wumpus_cfg.chan, "\2%s\2 has joined the game!", si->su->nick);
}

command_t wumpus_join = { "JOIN", "Joins the game.", AC_NONE, 0, cmd_join, { .path = "" } };

static void cmd_look(sourceinfo_t *si, int parc, char *parv[])
{
	player_t *p = find_player(si->su);

	if (p == NULL)
	{
		notice(wumpus_cfg.nick, si->su->nick, "You must be playing the game in order to use this command.");
		return;
	}

	if (!wumpus.running)
	{
		notice(wumpus_cfg.nick, si->su->nick, "You cannot use this command right now. Sorry.");
		return;
	}

	look_player(p);
}

command_t wumpus_look = { "LOOK", "View surroundings.", AC_NONE, 0, cmd_look, { .path = "" } };

static void cmd_move(sourceinfo_t *si, int parc, char *parv[])
{
	player_t *p = find_player(si->su);
	char *id = parv[0];

	if (!p)
	{
		notice(wumpus_cfg.nick, si->su->nick, "You must be playing the game in order to use this command.");
		return;
	}

	if (!id)
	{
		notice(wumpus_cfg.nick, si->su->nick, "You must provide a room to move to.");
		return;
	}

	if (!wumpus.running)
	{
		notice(wumpus_cfg.nick, si->su->nick, "The game must be running in order to use this command.");
		return;
	}

	move_player(p, atoi(id));
}

command_t wumpus_move = { "MOVE", "Move to another room.", AC_NONE, 1, cmd_move, { .path = "" } };

static void cmd_shoot(sourceinfo_t *si, int parc, char *parv[])
{
	player_t *p = find_player(si->su);
	char *id = parv[0];

	if (!p)
	{
		notice(wumpus_cfg.nick, si->su->nick, "You must be playing the game in order to use this command.");
		return;
	}

	if (!id)
	{
		notice(wumpus_cfg.nick, si->su->nick, "You must provide a room to shoot at.");
		return;
	}

	if (!wumpus.running)
	{
		notice(wumpus_cfg.nick, si->su->nick, "The game must be running in order to use this command.");
		return;
	}

	shoot_player(p, atoi(id));
}

command_t wumpus_shoot = { "SHOOT", "Shoot at another room.", AC_NONE, 1, cmd_shoot, { .path = "" } };

static void cmd_resign(sourceinfo_t *si, int parc, char *parv[])
{
	player_t *p = find_player(si->su);

	if (!p)
	{
		notice(wumpus_cfg.nick, si->su->nick, "You must be playing the game in order to use this command.");
		return;
	}

	if (!wumpus.running)
	{
		notice(wumpus_cfg.nick, si->su->nick, "The game must be running in order to use this command.");
		return;
	}

	msg(wumpus_cfg.nick, wumpus_cfg.chan, "\2%s\2 has quit the game!", p->u->nick);

	resign_player(p);
}

command_t wumpus_resign = { "RESIGN", "Resign from the game.", AC_NONE, 0, cmd_resign, { .path = "" } };

static void cmd_reset(sourceinfo_t *si, int parc, char *parv[])
{
	if (wumpus.running)
	{
		msg(wumpus_cfg.nick, wumpus_cfg.chan, "\2%s\2 has ended the game.", si->su->nick);

		end_game();

		wumpus.running = false;
		wumpus.starting = false;
	}
}

command_t wumpus_reset = { "RESET", "Resets the game.", AC_IRCOP, 0, cmd_reset, { .path = "" } };

static void cmd_help(sourceinfo_t *si, int parc, char *parv[])
{
	command_help(si, si->service->commands);
}

command_t wumpus_help = { "HELP", "Displays this command listing.", AC_NONE, 0, cmd_help, { .path = "help" } };

static void cmd_who(sourceinfo_t *si, int parc, char *parv[])
{
	mowgli_node_t *n;

	notice(wumpus_cfg.nick, si->su->nick, "The following people are playing:");

	MOWGLI_ITER_FOREACH(n, wumpus.players.head)
	{
		player_t *p = (player_t *) n->data;

		notice(wumpus_cfg.nick, si->su->nick, "- %s", p->u->nick);
	}
}

command_t wumpus_who = { "WHO", "Displays who is playing the game.", AC_NONE, 0, cmd_who, { .path = "" } };

/* removes quitting players */
static void
user_deleted(user_t *u)
{
	player_t *p;

	if ((p = find_player(u)) != NULL)
	{
		msg(wumpus_cfg.nick, wumpus_cfg.chan, "\2%s\2 has quit the game!", p->u->nick);
		resign_player(p);
	}
}

static void
burst_the_wumpus(void *unused)
{
	if (!wumpus.svs)
		wumpus.svs = service_add_static(wumpus_cfg.nick, wumpus_cfg.user, wumpus_cfg.host, wumpus_cfg.real, NULL);
	
	join(wumpus_cfg.chan, wumpus_cfg.nick);	/* what if we're not ready? then this is a NOOP */
}

/* start handler */
void
_modinit(module_t *m)
{
	if (cold_start)
	{
		hook_add_event("config_ready");
		hook_add_config_ready(burst_the_wumpus);
	}
	else
		burst_the_wumpus(NULL);

	hook_add_event("user_delete");
	hook_add_user_delete(user_deleted);

	service_bind_command(wumpus.svs, &wumpus_help);
	service_bind_command(wumpus.svs, &wumpus_start);
	service_bind_command(wumpus.svs, &wumpus_join);
	service_bind_command(wumpus.svs, &wumpus_move);
	service_bind_command(wumpus.svs, &wumpus_shoot);
	service_bind_command(wumpus.svs, &wumpus_resign);
	service_bind_command(wumpus.svs, &wumpus_reset);
	service_bind_command(wumpus.svs, &wumpus_who);
	service_bind_command(wumpus.svs, &wumpus_look);
}

void
_moddeinit(void)
{
	/* cleanup after ourselves if necessary */
	if (wumpus.running)
		end_game();

	service_delete(wumpus.svs);

	hook_del_user_delete(user_deleted);

	service_unbind_command(wumpus.svs, &wumpus_help);
	service_unbind_command(wumpus.svs, &wumpus_start);
	service_unbind_command(wumpus.svs, &wumpus_join);
	service_unbind_command(wumpus.svs, &wumpus_move);
	service_unbind_command(wumpus.svs, &wumpus_shoot);
	service_unbind_command(wumpus.svs, &wumpus_resign);
	service_unbind_command(wumpus.svs, &wumpus_reset);
	service_unbind_command(wumpus.svs, &wumpus_who);
	service_unbind_command(wumpus.svs, &wumpus_look);

	event_delete(move_wumpus, NULL);
	event_delete(start_game, NULL);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
