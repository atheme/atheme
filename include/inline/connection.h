/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2007 Pippijn van Steenhoven <pip88nl@gmail.com>
 */

#ifndef ATHEME_INC_INLINE_CONNECTION_H
#define ATHEME_INC_INLINE_CONNECTION_H 1

/*
 * connection_count()
 *
 * inputs:
 *       none
 *
 * outputs:
 *       number of connections tracked
 *
 * side effects:
 *       none
 */
static inline int connection_count(void)
{
	return MOWGLI_LIST_LENGTH(&connection_list);
}

#endif /* !ATHEME_INC_INLINE_CONNECTION_H */
