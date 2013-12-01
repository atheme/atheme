/*
 * gameserv/happyfarm
 *
 * Copyright (c) 2011 William Pitcock <nenolod@dereferenced.org>
 * Rights to this code are documented in doc/LICENSE.
 *
 * Grow crops, trade with and steal from your neighbors, and possibly sell your friends.
 */

#include "atheme.h"
#include "gameserv_common.h"

DECLARE_MODULE_V1("gameserv/happyfarm", false, _modinit, _moddeinit, PACKAGE_STRING, "Atheme Development Group <http://www.atheme.org>");

/* Privatedata schema... */
#define SCHEMA_KEY_HAPPYFARMER		"gameserv:happyfarm:farmer"

/*
 * Types of plants you may grow.
 */
typedef enum {
	PLANT_NOTHING = 0,
	PLANT_TURNIP,
	PLANT_TOMATO,
	PLANT_PARSLEY,
	PLANT_LETTUCE,
	PLANT_CORN,
	PLANT_POTATO,
	PLANT_BEANS,
	PLANT_PEACHES,
	PLANT_PEARS,
	PLANT_APPLES,
	PLANT_COUNT,
} happy_planttype_t;

typedef struct {
	unsigned int count;
} happy_inventory_t;

struct {
	const char *name;
	happy_planttype_t plant;
} happy_planttype_mapping[] = {
	{"nothing", PLANT_NOTHING},
	{"turnip", PLANT_TURNIP},
	{"tomato", PLANT_TOMATO},
	{"parsley", PLANT_PARSLEY},
	{"lettuce", PLANT_LETTUCE},
	{"corn", PLANT_CORN},
	{"potato", PLANT_POTATO},
	{"beans", PLANT_BEANS},
	{"peaches", PLANT_PEACHES},
	{"pears", PLANT_PEARS},
	{"apples", PLANT_APPLES},
	{NULL, PLANT_NOTHING},
};

/*
 * Really, we're unsure of the emotional state of the farmer.  However,
 * a sad farmer does not seem as amusing.  Plus, since IRC does not
 * convey emotion, it is safe to assume that the farmer is always
 * happy.
 */
typedef struct {
	/*
	 * A farm may be either personal, or collectively owned.  In Atheme terms,
	 * this means it's owned by an entity instead of a user class.  In terms of
	 * the Chairman Mao communist revolution of 1969, this means that some farms
	 * are the People's Happy Farms of Increased Profitability.  Either way,
	 * it means that we use a myentity_t as our derived base.
	 */
	myentity_t *owner;

	/*
	 * A farm, like any other kind of business entity has money.  Money is used
	 * to buy land and seeds.  You can also hire gangsters to go rob other players
	 * at knifepoint.  But that might make some people Sad Farmers.  And we wouldn't
	 * want that, or would we?
	 */
	int money;

	/*
	 * A farm consists of plots of land joined together.  Plots of land are planted
	 * with seeds, which cause plants to grow.  Then you sell your product for money.
	 * Some product sells for more than other products.  Anyway, this is a digital
	 * representation of a farm, so we store the farm plots in a double-ended queue.
	 */
	mowgli_list_t plots;

	/*
	 * A farm's most critical thing is it's Inventory.  According to the Ferengi
	 * rules of acquisition, Profit is Life.  Without an Inventory, there is
	 * no Profit.
	 */
	happy_inventory_t inventory[PLANT_COUNT];

	/*
	 * The second most important thing for a farm is it's seedbank.  No seedbank,
	 * means no Inventory, which means no Profit, which means Sad Starving Farmers.
	 */
	happy_inventory_t seed_inventory[PLANT_COUNT];
} happy_farmer_t;

/*
 * The game progresses at a rate of time that we will describe as The People's
 * Happy Time Unit For Gaming Satisfaction.  All time in this game is described as
 * being in units of The People's Happy Time Unit For Gaming Satisfaction.
 * The below tunable defines, in seconds, how long a Happy Time Unit For Gaming
 * Satisfaction actually is.
 */
#define PEOPLES_HAPPY_TIME_UNIT_FOR_GAMING_SATISFACTION		(60)

/*
 * And the below define is an alias so that I do not have to type out The People's
 * Happy Time Unit For Gaming Satisfaction a whole bunch.  Because I really don't
 * want to do that.
 */
#define TIME_UNIT_BASE		(PEOPLES_HAPPY_TIME_UNIT_FOR_GAMING_SATISFACTION)

/**********************************************************************************/

/*
 * Plots of land cost money.  This is the cost of the plot.
 */
#define PLOT_COST		(50)

/*
 * Plots of land grow over time.  We need to determine how long it takes for
 * a crop to grow.  I think that 7 Happy Time Units sounds good, so we'll make it 7.
 */
#define GROWTH_RATE		(7)

/*
 * The People's Glorious Land Office of Arbitration Management needs to keep a record
 * of all Happy Plots in the world.  We do so here.
 */
mowgli_list_t happy_plot_list = { NULL, NULL, 0 };

/*
 * A plot of land consists of seeds that grow into plants.  A plot is planted with X
 * number of seeds.  There is an upper limit on the number of seeds.
 */
#define MAX_SEEDS		(30)

/*
 * A plot of land, as previously mentioned, consists of seeds, which
 * grow into plants.  Once GROWTH_RATE is reached, the plot may be harvested
 * and replotted.
 */
typedef struct {
	/*
	 * As previously mentioned, a plot of land contains seeds.  All seeds on a 
	 * plot of land are the same type.  So, plot::plant == PLANT_CORN means
	 * you're growing corn.
	 */
	happy_planttype_t plant;

	/*
	 * Since plots of land contain seeds, we need to know how many seeds we have.
	 */
	unsigned int count;

	/*
	 * A node for storage in the farmer's plot list.
	 */
	mowgli_node_t farmer_node;

	/*
	 * A nore for storage in the global plot list.
	 */
	mowgli_node_t global_node;
} happy_plot_t;

/*******************************************************************************************/

/*
 * Our happy farmers are happily stored in a magazine allocator to be quickly allocated to
 * happy users.  This allows for maximum efficiency in gameplay success.
 */
mowgli_heap_t *farmer_heap = NULL;

/*
 * We need to have a magazine allocator for plots too.
 */
mowgli_heap_t *plot_heap = NULL;

/*
 * When a happy farmer joins our happy little game, we have to create a happy_farmer_t
 * object for them.  Which, you know, requires a constructor...
 */
static happy_farmer_t *happy_farmer_create(myentity_t * mt)
{
	happy_farmer_t *farmer;

	return_val_if_fail(mt != NULL, NULL);

	farmer = mowgli_heap_alloc(farmer_heap);
	farmer->owner = mt;
	farmer->money = 100;

	privatedata_set(farmer->owner, SCHEMA_KEY_HAPPYFARMER, farmer);

	return farmer;
}

/*
 * When a happy farmer decides to commit suicide, Government must intervene and start the
 * probate procedures (which in our case means we take all resources back).
 */
static void happy_farmer_destroy(happy_farmer_t * farmer)
{
	mowgli_node_t *n, *tn;

	return_if_fail(farmer != NULL);

	MOWGLI_ITER_FOREACH_SAFE(n, tn, farmer->plots.head)
	{
		happy_plot_t *plot = n->data;

		mowgli_node_delete(&plot->farmer_node, &farmer->plots);
		mowgli_node_delete(&plot->global_node, &happy_plot_list);

		mowgli_heap_free(plot_heap, plot);
	}

	privatedata_set(farmer->owner, SCHEMA_KEY_HAPPYFARMER, NULL);

	mowgli_heap_free(farmer_heap, farmer);
}

/*
 * A farmer just bought a plot of land.
 */
static happy_plot_t *happy_plot_create(happy_farmer_t * farmer)
{
	happy_plot_t *plot;

	return_val_if_fail(farmer != NULL, NULL);

	plot = mowgli_heap_alloc(plot_heap);
	mowgli_node_add(plot, &plot->farmer_node, &farmer->plots);
	mowgli_node_add(plot, &plot->global_node, &happy_plot_list);

	return plot;
}

/*
 * The farmer has sold his plot of land back.
 */
static void happy_plot_destroy(happy_farmer_t * farmer, happy_plot_t * plot)
{
	return_if_fail(farmer != NULL);
	return_if_fail(plot != NULL);

	mowgli_node_delete(&plot->farmer_node, &farmer->plots);
	mowgli_node_delete(&plot->global_node, &happy_plot_list);

	mowgli_heap_free(plot_heap, plot);
}

/*
 * Find the first plot that can actually be sold back.
 */
static happy_plot_t *happy_farmer_find_vacant_plot(happy_farmer_t * farmer)
{
	mowgli_node_t *n;

	return_val_if_fail(farmer != NULL, NULL);

	MOWGLI_ITER_FOREACH(n, farmer->plots.head)
	{
		happy_plot_t *plot = n->data;

		if (plot->plant == PLANT_NOTHING)
			return plot;
	}

	return NULL;
}

/*
 * Determine a plant type, given user input.
 */
static happy_planttype_t happy_plant_by_name(const char *name)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(happy_planttype_mapping); i++)
	{
		if (!strcasecmp(happy_planttype_mapping[i].name, name))
			return happy_planttype_mapping[i].plant;
	}

	return PLANT_NOTHING;
}

/*******************************************************************************************/

/*
 * Syntax: JOIN
 * Result: Player joins the game.
 */
static void __command_join(sourceinfo_t * si, int parc, char *parv[])
{
	happy_farmer_t *farmer;

	farmer = happy_farmer_create(entity(si->smu));

	command_success_nodata(si, _("Welcome to Happy Farm!  May your farm be lucky."));
	command_success_nodata(si, _("You have started with \2%d\2 money.  For help, use \2/msg %s HELP HAPPYFARM\2."), farmer->money, si->service->nick);
}

command_t command_join = { "JOIN", N_("Join the Happy Farm game!"), AC_AUTHENTICATED, 0, __command_join, { .path = "gameserv/happyfarm_join" } };

/*
 * Syntax: BUYPLOT
 * Result: Player buys a plot if they have enough money.
 */
static void __command_buyplot(sourceinfo_t * si, int parc, char *parv[])
{
	myentity_t *mt;
	happy_farmer_t *farmer;

	mt = entity(si->smu);

	return_if_fail(mt != NULL);

	farmer = privatedata_get(mt, SCHEMA_KEY_HAPPYFARMER);
	if (farmer == NULL)
	{
		command_fail(si, fault_noprivs, _("You do not appear to be playing Happy Farm.  To join the game, use \2/msg %s HAPPYFARM JOIN\2."), si->service->nick);
		return;
	}

	/* check if the farmer has enough money. */
	if (farmer->money < PLOT_COST)
	{
		command_fail(si, fault_noprivs, _("You don't have enough money to buy a plot of land."));
		return;
	}

	farmer->money -= PLOT_COST;
	happy_plot_create(farmer);

	command_success_nodata(si, _("You have bought a plot of land!"));
	command_success_nodata(si, _("You have \2%d\2 money available."), farmer->money);
}

command_t command_buyplot = { "BUYPLOT", N_("Buy a plot of land!"), AC_AUTHENTICATED, 0, __command_buyplot, { .path = "gameserv/happyfarm_buyplot" } };

/*
 * Syntax: SELLPLOT
 * Result: Player sells a plot back to the motherland
 */
static void __command_sellplot(sourceinfo_t * si, int parc, char *parv[])
{
	myentity_t *mt;
	happy_farmer_t *farmer;
	happy_plot_t *plot;

	mt = entity(si->smu);

	return_if_fail(mt != NULL);

	farmer = privatedata_get(mt, SCHEMA_KEY_HAPPYFARMER);
	if (farmer == NULL)
	{
		command_fail(si, fault_noprivs, _("You do not appear to be playing Happy Farm.  To join the game, use \2/msg %s HAPPYFARM JOIN\2."), si->service->nick);
		return;
	}

	if ((plot = happy_farmer_find_vacant_plot(farmer)) == NULL)
	{
		command_fail(si, fault_noprivs, _("You do not have any vacant plots at this time."));
		return;
	}

	farmer->money += PLOT_COST / 2;
	happy_plot_destroy(farmer, plot);

	command_success_nodata(si, _("You have sold a plot of land."));
	command_success_nodata(si, _("You have \2%d\2 money available."), farmer->money);
}

command_t command_sellplot = { "SELLPLOT", N_("Sell a vacant plot of land."), AC_AUTHENTICATED, 0, __command_sellplot, { .path = "gameserv/happyfarm_sellplot" } };

/*******************************************************************************************/

mowgli_patricia_t *happyfarm_cmd_subtree = NULL;

static void __command_trampoline(sourceinfo_t * si, int parc, char *parv[])
{
	char *subcmd = parv[0];
	command_t *c;

	if (subcmd == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "HAPPYFARM");
		command_fail(si, fault_needmoreparams, _("Syntax: HAPPYFARM <command>"));
		return;
	}

	/* take the command through the hash table */
	if ((c = command_find(happyfarm_cmd_subtree, subcmd)))
	{
		command_exec(si->service, si, c, parc - 1, parv + 1);
	}
	else
	{
		command_fail(si, fault_badparams, _("Invalid command. Use \2/%s%s HELP HAPPYFARM\2 for a command listing."), (ircd->uses_rcommand == false) ? "msg " : "", si->service->nick);
	}
}

command_t command_happyfarm = { "HAPPYFARM", N_("Happy Farm!"), AC_AUTHENTICATED, 2, __command_trampoline, { .path = "gameserv/happyfarm" } };

/*******************************************************************************************/

void _modinit(module_t * m)
{
	farmer_heap = mowgli_heap_create(sizeof(happy_farmer_t), 32, BH_LAZY);
	plot_heap = mowgli_heap_create(sizeof(happy_plot_t), 32, BH_LAZY);

	service_named_bind_command("gameserv", &command_happyfarm);

	happyfarm_cmd_subtree = mowgli_patricia_create(strcasecanon);

	command_add(&command_join, happyfarm_cmd_subtree);
	command_add(&command_buyplot, happyfarm_cmd_subtree);
	command_add(&command_sellplot, happyfarm_cmd_subtree);
}

void _moddeinit(module_unload_intent_t intent)
{
	command_delete(&command_join, happyfarm_cmd_subtree);
	command_delete(&command_buyplot, happyfarm_cmd_subtree);
	command_delete(&command_sellplot, happyfarm_cmd_subtree);

	mowgli_patricia_destroy(happyfarm_cmd_subtree, NULL, NULL);

	service_named_unbind_command("gameserv", &command_happyfarm);

	mowgli_heap_destroy(farmer_heap);
	mowgli_heap_destroy(plot_heap);
}
