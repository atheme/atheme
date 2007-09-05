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
	return LIST_LENGTH(&connection_list);
}

/*
 * connection_write_raw()
 *
 * inputs:
 *       connection_t to write to, raw string to write
 *
 * outputs:
 *       none
 *
 * side effects:
 *       data is added to the connection_t sendq cache.
 */
static inline void connection_write_raw(connection_t *to, char *data)
{
	sendq_add(to, data, strlen(data));
}

#endif
