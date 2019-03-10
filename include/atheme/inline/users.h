/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 */

#ifndef ATHEME_INC_INLINE_USERS_H
#define ATHEME_INC_INLINE_USERS_H 1

#include <atheme/stdheaders.h>
#include <atheme/users.h>

static inline bool
user_is_channel_banned(struct user *const restrict u, const char ban_type)
{
	if (find_user_banned_channel(u, ban_type))
		return true;

	return false;
}

#endif /* !ATHEME_INC_INLINE_USERS_H */
