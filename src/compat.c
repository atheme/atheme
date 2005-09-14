/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Compatibility with Atheme 0.2 block allocator (read: craq++)
 *
 * $Id: compat.c 2235 2005-09-14 07:29:13Z nenolod $
 */

#include "atheme.h"

BlockHeap *BlockHeapCreate(size_t elemsize, int elemsperblock)
{
	static char buf[BUFSIZE];

	/* please no IOCCC entries --nenolod */
	strlcpy(buf, "unnamed_heap-", BUFSIZE);
	strlcat(buf, gen_pw(16), BUFSIZE);

	return (BlockHeap *) bcreate(buf, elemsize, elemsperblock);
}

void *BlockHeapAlloc(BlockHeap *bh)
{
	return balloc((heap_t *) bh);
}

int BlockHeapFree(BlockHeap *bh, void *ptr)
{
	return bfree((heap_t *) bh, ptr) ? 0 : 1;
}

int BlockHeapDestroy(BlockHeap *bh)
{
	return bdelete((heap_t *) bh) ? 0 : 1;
}

void BlockHeapUsage(BlockHeap *bh, size_t *bused, size_t *bfreem, size_t *bmemusage)
{
	bstat((heap_t *) bh, bused, bfreem, bmemusage);
}

