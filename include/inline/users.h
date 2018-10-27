#ifndef ATHEME_INC_INLINE_USERS_H
#define ATHEME_INC_INLINE_USERS_H 1

static inline bool
user_is_channel_banned(struct user *const restrict u, const char ban_type)
{
	if (find_user_banned_channel(u, ban_type))
		return true;

	return false;
}

#endif /* !ATHEME_INC_INLINE_USERS_H */
