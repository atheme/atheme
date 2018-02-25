/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Memory stuff.
 */

#ifndef ATHEME_INC_ATHEME_MEMORY_H
#define ATHEME_INC_ATHEME_MEMORY_H 1

void *smalloc(size_t len);
void *scalloc(size_t num, size_t len);
void *srealloc(void *ptr, size_t len);
char *sstrdup(const char *ptr);
char *sstrndup(const char *ptr, size_t len);

#endif /* !ATHEME_INC_ATHEME_MEMORY_H */
