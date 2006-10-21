/*
 * Copyright (c) 2006 Atheme developers
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Services ignorelist functions.
 *
 * $Id: svsignore.c 6791 2006-10-21 16:59:20Z jilles $
 */

#include "atheme.h"

list_t svs_ignore_list;

svsignore_t *svsignore_add(char *mask, char *reason)
{
        svsignore_t *svsignore;
        node_t *n = node_create();

        svsignore = smalloc(sizeof(svsignore_t));
                        
        node_add(svsignore, n, &svs_ignore_list);
        
        svsignore->mask = sstrdup(mask);
        svsignore->settime = CURRTIME;
        svsignore->reason = sstrdup(reason);
        cnt.svsignore++;
         
        return svsignore;
}
                
svsignore_t *svsignore_find(user_t *source)
{       
        svsignore_t *svsignore;
        node_t *n;
        char host[BUFSIZE];

	if (!use_svsignore)
		return NULL;

        *host = '\0';
        strlcpy(host, source->nick, BUFSIZE);
        strlcat(host, "!", BUFSIZE);
        strlcat(host, source->user, BUFSIZE);
        strlcat(host, "@", BUFSIZE);
        strlcat(host, source->host, BUFSIZE);
                
        LIST_FOREACH(n, svs_ignore_list.head)
        {
                svsignore = (svsignore_t *)n->data;
        
                if (!match(svsignore->mask, host))
                        return svsignore;
        }

        return NULL;
}

void svsignore_delete(svsignore_t *svsignore)
{
	node_t *n;

	n = node_find(svsignore, &svs_ignore_list);
	node_del(n, &svs_ignore_list);

	free(svsignore->mask);
	free(svsignore->reason);
	free(svsignore);
}
