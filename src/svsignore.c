/*
 * Copyright (c) 2006 Atheme developers
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Services ignorelist functions.
 *
 * $Id: svsignore.c 7779 2007-03-03 13:55:42Z pippijn $
 */

#include "atheme.h"

list_t svs_ignore_list;

/*
 * svsignore_add(char *mask, char *reason)
 *
 * Services ignore factory.
 *
 * Inputs:
 *     - mask to ignore
 *     - reason for ignore
 *
 * Outputs:
 *     - on success, a new svsignore object
 *
 * Side Effects:
 *     - a services ignore is added
 *
 * Bugs:
 *     - this function does not check for dupes
 */
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

/*
 * svsignore_find(user_t *source)
 *
 * Finds any services ignores that affect a user.
 *
 * Inputs:
 *     - user object to check
 *
 * Outputs:
 *     - if any ignores match, the ignore that matches
 *     - if none match, NULL
 *
 * Side Effects:
 *     - none
 */                
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

/*
 * svsignore_delete(svsignore_t *svsignore)
 *
 * Destroys a services ignore.
 *
 * Inputs:
 *     - svsignore to destroy
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - a services ignore is destroyed and removed from the list
 */
void svsignore_delete(svsignore_t *svsignore)
{
	node_t *n;

	n = node_find(svsignore, &svs_ignore_list);
	node_del(n, &svs_ignore_list);

	free(svsignore->mask);
	free(svsignore->reason);
	free(svsignore);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
