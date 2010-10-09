/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures and macros for manipulating linked lists.
 *
 */

#ifndef NODE_H
#define NODE_H

typedef mowgli_node_t node_t;

E mowgli_list_t *list_create(void);
E void list_free(mowgli_list_t *l);

E node_t *node_create(void);
E void node_free(node_t *n);
E void node_add(void *data, node_t *n, mowgli_list_t *l);
E void node_add_head(void *data, node_t *n, mowgli_list_t *l);
E void node_add_before(void *data, node_t *n, mowgli_list_t *l, node_t *before);
E void node_del(node_t *n, mowgli_list_t *l);
E node_t *node_find(void *data, mowgli_list_t *l);
E void node_move(node_t *m, mowgli_list_t *oldlist, mowgli_list_t *newlist);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
