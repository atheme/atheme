#ifndef INLINE_ACCOUNT_H
#define INLINE_ACCOUNT_H

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
static inline myuser_t *myuser_find(const char *name)
{
	return name ? user(myentity_find(name)) : NULL;
}

static inline myuser_t *myuser_find_uid(const char *uid)
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
static inline mynick_t *mynick_find(const char *name)
{
	return name ? mowgli_patricia_retrieve(nicklist, name) : NULL;
}

static inline myuser_t *myuser_find_by_nick(const char *name)
{
	myuser_t *mu;
	mynick_t *mn;

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
static inline myuser_name_t *myuser_name_find(const char *name)
{
	return name ? mowgli_patricia_retrieve(oldnameslist, name) : NULL;
}

static inline mychan_t *mychan_find(const char *name)
{
	return name ? mowgli_patricia_retrieve(mclist, name) : NULL;
}

static inline bool chanacs_entity_has_flag(mychan_t *mychan, myentity_t *mt, unsigned int level)
{
	return mychan && mt ? (chanacs_entity_flags(mychan, mt) & level) != 0 : false;
}

static inline bool chanacs_source_has_flag(mychan_t *mychan, sourceinfo_t *si, unsigned int level)
{
	return si->su != NULL ? chanacs_user_has_flag(mychan, si->su, level) :
		chanacs_entity_has_flag(mychan, entity(si->smu), level);
}

/* Destroy a chanacs if it has no flags */
static inline void chanacs_close(chanacs_t *ca)
{
	if (ca->level == 0)
		object_unref(ca);
}

/* Call this with a chanacs_t with level==0 */
static inline bool chanacs_is_table_full(chanacs_t *ca)
{
	return chansvs.maxchanacs > 0 &&
		MOWGLI_LIST_LENGTH(&ca->mychan->chanacs) > chansvs.maxchanacs;
}

#endif
