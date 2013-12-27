/*
 * Copyright (c) 2010 William Pitcock <nenolod@atheme.org>.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Management of tainted running configuration reasons and status.
 */

#ifndef ATHEME_TAINT_H
#define ATHEME_TAINT_H

typedef struct {
	char condition[BUFSIZE];
	char file[BUFSIZE];
	int line;
	char buf[BUFSIZE];
	mowgli_node_t node;
} taint_reason_t;

E mowgli_list_t taint_list;

#define IS_TAINTED	MOWGLI_LIST_LENGTH(&taint_list)
#define TAINT_ON(cond, reason) \
	if ((cond))						\
	{							\
		taint_reason_t *tr;				\
		tr = scalloc(sizeof(taint_reason_t), 1);	\
		mowgli_strlcpy(tr->condition, #cond, BUFSIZE);	\
		mowgli_strlcpy(tr->file, __FILE__, BUFSIZE);	\
		tr->line = __LINE__;				\
		mowgli_strlcpy(tr->buf, (reason), BUFSIZE);	\
		mowgli_node_add(tr, &tr->node, &taint_list);	\
		slog(LG_ERROR, "TAINTED: %s", (reason));	\
		if (!config_options.allow_taint)		\
		{						\
			slog(LG_ERROR, "exiting due to taint");	\
			exit(EXIT_FAILURE);			\
		}						\
	}

#endif
