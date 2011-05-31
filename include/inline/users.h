#ifndef INLINE_USERS_H
#define INLINE_USERS_H

/*
 * user_recent_account(user_t *u)
 *
 * Inputs:
 *      - user_t object
 *
 * Outputs:
 *	- most recent myuser_t account logged into
 *
 * Side Effects:
 *	- none
 */
static inline myuser_t *
user_recent_account(user_t *u)
{
	return_val_if_fail(u != NULL, NULL);

	if (u->logins.tail != NULL)
		return u->logins.tail->data;

	return NULL;
}

#endif
