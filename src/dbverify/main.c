/*
 * Copyright (c) 2012 William Pitcock <nenolod@dereferenced.org>.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "atheme.h"
#include "libathemecore.h"

static unsigned int verify_entity_uids(void)
{
	unsigned int errcnt = 0;
	mowgli_patricia_t *known = mowgli_patricia_create(strcasecanon);
	myentity_iteration_state_t state;
	myentity_t *mt;

	MYENTITY_FOREACH_T(mt, &state, ENT_ANY)
	{
		myentity_t *mt2;

		if ((mt2 = mowgli_patricia_retrieve(known, mt->id)) != NULL)
		{
			mowgli_strlcpy(mt->id, myentity_alloc_uid(), sizeof mt->id);

			slog(LG_INFO, "*** phase 4: entity '%s' has duplicate EID '%s' (belonging to '%s'); regenerating as '%s'",
					mt->name, mt2->id, mt2->name, mt->id);

			errcnt++;
			continue;
		}

		mowgli_patricia_add(known, mt->id, mt);
	}

	mowgli_patricia_destroy(known, NULL, NULL);
	return errcnt;
}

static void verify_channel_registrations(void)
{
	mowgli_patricia_iteration_state_t state;
	mychan_t *mc;

	MOWGLI_PATRICIA_FOREACH(mc, &state, mclist)
	{
		mowgli_node_t *n, *tn;
		mowgli_patricia_t *known = mowgli_patricia_create(strcasecanon);

		slog(LG_INFO, "*** phase 3: checking %s for consistency", mc->name);

		MOWGLI_ITER_FOREACH_SAFE(n, tn, mc->chanacs.head)
		{
			chanacs_t *ca = n->data, *ca2;
			stringref key = ca->entity != NULL ? ca->entity->name : ca->host;

			if (key == NULL)
			{
				slog(LG_INFO, "*** phase 3: %s: chanacs entry %p is dangling; unlinking from object store", mc->name, ca);
				mowgli_node_delete(&ca->cnode, &mc->chanacs);
				continue;
			}

			if ((ca2 = mowgli_patricia_retrieve(known, key)) != NULL)
			{
				slog(LG_INFO, "*** phase 3: %s: chanacs entry '%s' (%p) duplicates chanacs entry %p", mc->name, ca->entity != NULL ? ca->entity->name : ca->host, ca, ca2);
				mowgli_node_delete(&ca->cnode, &mc->chanacs);
				continue;
			}

			if ((ca->level & CA_AKICK) && ca->level != CA_AKICK)
			{
				unsigned int flags = ca->level & ~CA_AKICK;

				ca->level = CA_AKICK;
				slog(LG_INFO, "*** phase 3: %s: chanacs entry '%s' (%p) is an AKICK but has other flags -- removing %s from it",
				     mc->name, ca->entity != NULL ? ca->entity->name : ca->host, ca, bitmask_to_flags(flags));
			}

			mowgli_patricia_add(known, key, ca);
		}

		mowgli_patricia_destroy(known, NULL, NULL);
	}
}

static void handle_mdep(database_handle_t *db, const char *type)
{
	const char *modname = db_sread_word(db);

	module_load(modname);
}

int main(int argc, char *argv[])
{
	atheme_bootstrap();
	atheme_init(argv[0], LOGDIR "/dbverify.log");
	atheme_setup();
	module_t *m;
	unsigned int errcnt;
	char *filename = argv[1] ? argv[1] : "services.db";

	runflags = RF_LIVE;
	datadir = DATADIR;
	strict_mode = false;
	offline_mode = true;

	slog(LG_INFO, "dbverify is operating on %s", filename);

	m = module_load("backend/opensex");

	db_unregister_type_handler("MDEP");
	db_register_type_handler("MDEP", handle_mdep);

	slog(LG_INFO, "*** phase 1: demarshaling objects from opensex datastore");

	runflags &= ~RF_LIVE;
	db_load(filename);
	runflags |= RF_LIVE;

	slog(LG_INFO, "*** phase 2: doing basic atheme database consistency check");

	db_check();

	slog(LG_INFO, "*** phase 3: verifying channel registration integrity");

	verify_channel_registrations();

	slog(LG_INFO, "*** phase 4: verifying entity UID integrity");

	while ((errcnt = verify_entity_uids()) != 0)
		slog(LG_INFO, "*** phase 4: %u error(s) were found; running another pass", errcnt);

	slog(LG_INFO, "*** phase 5: writing corrected state to object store");

	db_save(filename);

	return EXIT_SUCCESS;
}
