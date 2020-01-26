/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2010 William Pitcock <nenolod@dereferenced.org>
 *
 * Management of tainted running configuration reasons and status.
 */

#ifndef ATHEME_INC_TAINT_H
#define ATHEME_INC_TAINT_H 1

#include <atheme/constants.h>
#include <atheme/stdheaders.h>

struct taint_reason
{
	mowgli_node_t   node;
	char            buf[BUFSIZE];
	char            condition[BUFSIZE];
	char            file[BUFSIZE];
	unsigned int    line;
};

extern mowgli_list_t taint_list;

#define IS_TAINTED      MOWGLI_LIST_LENGTH(&taint_list)

#define TAINT_ON(cond, reason)                                                          \
        if ((cond))                                                                     \
        {                                                                               \
                struct taint_reason *const tr = smalloc(sizeof *tr);                    \
                (void) mowgli_strlcpy(tr->buf, (reason), sizeof tr->buf);               \
                (void) mowgli_strlcpy(tr->condition, #cond, sizeof tr->condition);      \
                (void) mowgli_strlcpy(tr->file, __FILE__, sizeof tr->file);             \
                tr->line = (unsigned int) __LINE__;                                     \
                (void) mowgli_node_add(tr, &tr->node, &taint_list);                     \
                (void) slog(LG_ERROR, "TAINTED: %s", (reason));                         \
                if (! config_options.allow_taint)                                       \
                {                                                                       \
                        slog(LG_ERROR, "exiting due to taint");                         \
                        exit(EXIT_FAILURE);                                             \
                }                                                                       \
        }

#endif /* !ATHEME_INC_TAINT_H */
