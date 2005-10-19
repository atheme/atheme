/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures and macros for manipulating linked lists.
 *
 * $Id: dlink.h 3023 2005-10-19 05:26:32Z nenolod $
 */

#ifndef NODE_H
#define NODE_H

typedef struct node_ node_t;
typedef struct list_ list_t;

/* macros for linked lists */
#define LIST_FOREACH(n, head) for (n = (head); n; n = n->next)  
#define LIST_FOREACH_NEXT(n, head) for (n = (head); n->next; n = n->next)
#define LIST_FOREACH_PREV(n, tail) for (n = (tail); n; n = n->prev)

#define LIST_LENGTH(list) (list)->count

#define LIST_FOREACH_SAFE(n, tn, head) for (n = (head), tn = n ? n->next : NULL; n != NULL; n = tn, tn = n ? n->next : NULL)

/* list node struct */
struct node_
{
  node_t *next, *prev; 
  void *data;                   /* pointer to real structure */
};

/* node list struct */
struct list_
{
  node_t *head, *tail;
  uint32_t count;                    /* how many entries in the list */
};

E node_t *node_create(void);
E void node_free(node_t *n);
E void node_add(void *data, node_t *n, list_t *l);
E void node_del(node_t *n, list_t *l);
E node_t *node_find(void *data, list_t *l);
E void node_move(node_t *m, list_t *oldlist, list_t *newlist);

#endif
