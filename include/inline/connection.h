#ifndef INLINE_CONNECTION_H
#define INLINE_CONNECTION_H

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

#endif
