/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2007 Pippijn van Steenhoven <pip88nl@gmail.com>
 */

#ifndef ATHEME_INC_INLINE_ACCOUNT_H
#define ATHEME_INC_INLINE_ACCOUNT_H 1

#include <atheme/account.h>
#include <atheme/channels.h>
#include <atheme/entity.h>
#include <atheme/object.h>
#include <atheme/services.h>
#include <atheme/sourceinfo.h>
#include <atheme/stdheaders.h>

/*
 * myuser_find(const char *name)
 *
 * Retrieves an account from the accounts DTree.
 *
 * Inputs:
 *      - account name to retrieve
 *
 * Outputs:
 *      - account wanted or NULL if it's not in the DTree.
 *
 * Side Effects:
 *      - none
 */
static inline struct myuser *myuser_find(const char *name)
{
	return name ? user(myentity_find(name)) : NULL;
}

static inline struct myuser *myuser_find_uid(const char *uid)
{
	return uid ? user(myentity_find_uid(uid)) : NULL;
}

/*
 * mynick_find(const char *name)
 *
 * Retrieves a nick from the nicks DTree.
 *
 * Inputs:
 *      - nickname to retrieve
 *
 * Outputs:
 *      - nick wanted or NULL if it's not in the DTree.
 *
 * Side Effects:
 *      - none
 */
static inline struct mynick *mynick_find(const char *name)
{
	return name ? mowgli_patricia_retrieve(nicklist, name) : NULL;
}

static inline struct myuser *myuser_find_by_nick(const char *name)
{
	struct myuser *mu;
	struct mynick *mn;

	mu = myuser_find(name);
	if (mu != NULL)
		return mu;

	if (!nicksvs.no_nick_ownership)
	{
		mn = mynick_find(name);
		if (mn != NULL)
			return mn->owner;
	}

	return NULL;
}

/*
 * myuser_name_find(const char *name)
 *
 * Retrieves a record from the oldnames DTree.
 *
 * Inputs:
 *      - account name to retrieve
 *
 * Outputs:
 *      - record wanted or NULL if it's not in the DTree.
 *
 * Side Effects:
 *      - none
 */
static inline struct myuser_name *myuser_name_find(const char *name)
{
	return name ? mowgli_patricia_retrieve(oldnameslist, name) : NULL;
}

static inline struct mychan *mychan_find(const char *name)
{
	return name ? mowgli_patricia_retrieve(mclist, name) : NULL;
}

static inline struct mychan *mychan_from(struct channel *chan)
{
	return chan ? (chan->mychan ? chan->mychan : mychan_find(chan->name)) : NULL;
}

static inline bool chanacs_entity_has_flag(struct mychan *mychan, struct myentity *mt, unsigned int level)
{
	return mychan && mt ? (chanacs_entity_flags(mychan, mt) & level) != 0 : false;
}

static inline bool chanacs_source_has_flag(struct mychan *mychan, struct sourceinfo *si, unsigned int level)
{
	return si->su != NULL ? chanacs_user_has_flag(mychan, si->su, level) :
		chanacs_entity_has_flag(mychan, entity(si->smu), level);
}

/* Destroy a chanacs if it has no flags */
static inline void chanacs_close(struct chanacs *ca)
{
	if (ca->level == 0)
		atheme_object_unref(ca);
}

/* Call this with a struct chanacs -> level == 0 */
static inline bool chanacs_is_table_full(struct chanacs *ca)
{
	return chansvs.maxchanacs > 0 &&
		MOWGLI_LIST_LENGTH(&ca->mychan->chanacs) > chansvs.maxchanacs;
}

#endif /* !ATHEME_INC_INLINE_ACCOUNT_H */
