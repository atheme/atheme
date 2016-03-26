/*
 * Copyright (c) 2011 William Pitcock <nenolod@dereferenced.org>
 *
 * Rights to this code are as documented in doc/LICENSE.
 */

#include "atheme.h"
#include "exttarget.h"

DECLARE_MODULE_V1
(
	"exttarget/server", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	VENDOR_STRING
);

static mowgli_patricia_t **exttarget_tree = NULL;

typedef struct {
	myentity_t parent;
	stringref server;
} server_exttarget_t;

static chanacs_t *server_ext_match_user(chanacs_t *ca, user_t *u)
{
	server_exttarget_t *ent;
	mowgli_node_t *n;

	ent = (server_exttarget_t *) ca->entity;
	if (match(ent->server, u->server->name) == 0)
		return ca;

	return NULL;
}

static chanacs_t *server_ext_match_entity(chanacs_t *ca, myentity_t *mt)
{
	if (ca->entity == mt)
		return ca;

	return NULL;
}

static bool server_ext_can_register_channel(myentity_t *mt)
{
	return false;
}

static bool server_ext_allow_foundership(myentity_t *mt)
{
	return false;
}

static entity_chanacs_validation_vtable_t server_ext_validate = {
	.match_entity = server_ext_match_entity,
	.match_user = server_ext_match_user,
	.can_register_channel = server_ext_can_register_channel,
	.allow_foundership = server_ext_allow_foundership,
};

static mowgli_heap_t *server_ext_heap = NULL;
static mowgli_patricia_t *server_exttarget_tree = NULL;

static void server_ext_delete(server_exttarget_t *e)
{
	return_if_fail(e != NULL);

	mowgli_patricia_delete(server_exttarget_tree, e->server);
	strshare_unref(e->server);
	strshare_unref(entity(e)->name);

	mowgli_heap_free(server_ext_heap, e);
}

static myentity_t *server_validate_f(const char *param)
{
	char *name;
	server_exttarget_t *ext;
	size_t namelen;

	if (param == NULL)
		return NULL;

	if (*param == '\0')
		return NULL;

	/* if we already have an object, return it from our tree. */
	if ((ext = mowgli_patricia_retrieve(server_exttarget_tree, param)) != NULL)
		return entity(ext);

	ext = mowgli_heap_alloc(server_ext_heap);
	ext->server = strshare_get(param);

	/* name the entity... $server:param */
#define NAMEPREFIX "$server:"
	namelen = sizeof NAMEPREFIX + strlen(param);

	name = smalloc(namelen);
	memcpy(name, NAMEPREFIX, sizeof NAMEPREFIX - 1);
	memcpy(name + sizeof NAMEPREFIX - 1, param, namelen - sizeof NAMEPREFIX + 1);

	entity(ext)->name = strshare_get(name);
	free(name);
#undef NAMEPREFIX

	/* hook up the entity's validation table. */
	entity(ext)->chanacs_validate = &server_ext_validate;
	entity(ext)->type = ENT_EXTTARGET;

	/* initialize the object. */
	object_init(object(ext), entity(ext)->name, (destructor_t) server_ext_delete);

	/* add the object to the exttarget tree. */
	mowgli_patricia_add(server_exttarget_tree, ext->server, ext);

	/* return the object as initially unowned by sinking the reference count. */
	return object_sink_ref(ext);
}

void _modinit(module_t *m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, exttarget_tree, "exttarget/main", "exttarget_tree");

	mowgli_patricia_add(*exttarget_tree, "server", server_validate_f);

	server_exttarget_tree = mowgli_patricia_create(irccasecanon);
	server_ext_heap = mowgli_heap_create(sizeof(server_exttarget_t), 32, BH_LAZY);
}

void _moddeinit(module_unload_intent_t intent)
{
	mowgli_heap_destroy(server_ext_heap);
	mowgli_patricia_delete(*exttarget_tree, "server");
	mowgli_patricia_destroy(server_exttarget_tree, NULL, NULL);
}
