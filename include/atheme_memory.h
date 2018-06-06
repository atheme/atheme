/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005 Atheme Project (http://atheme.org/)
 *
 * Memory stuff.
 */

#ifndef ATHEME_INC_ATHEME_MEMORY_H
#define ATHEME_INC_ATHEME_MEMORY_H 1

void *smalloc(size_t len) ATHEME_FATTR_MALLOC;
void *scalloc(size_t num, size_t len) ATHEME_FATTR_MALLOC;
void *srealloc(void *ptr, size_t len);
char *sstrdup(const char *ptr) ATHEME_FATTR_MALLOC;
char *sstrndup(const char *ptr, size_t len) ATHEME_FATTR_MALLOC;

#endif /* !ATHEME_INC_ATHEME_MEMORY_H */
