/*
 * atheme-services: A collection of minimalist IRC services
 * svsignore.c: Services ignore list management.
 *
 * Copyright (c) 2005-2007 Atheme Project (http://www.atheme.org)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "atheme.h"

mowgli_list_t svs_ignore_list;

/*
 * svsignore_add(const char *mask, const char *reason)
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
struct svsignore * ATHEME_FATTR_MALLOC
svsignore_add(const char *mask, const char *reason)
{
        struct svsignore *const svsignore = smalloc(sizeof *svsignore);

        svsignore->mask = sstrdup(mask);
        svsignore->settime = CURRTIME;
        svsignore->reason = sstrdup(reason);

        mowgli_node_t *n = mowgli_node_create();
        mowgli_node_add(svsignore, n, &svs_ignore_list);

        cnt.svsignore++;
        return svsignore;
}

/*
 * svsignore_find(struct user *source)
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
struct svsignore *
svsignore_find(struct user *source)
{
        struct svsignore *svsignore;
        mowgli_node_t *n;
        char host[BUFSIZE];

	if (!use_svsignore)
		return NULL;

        *host = '\0';
        mowgli_strlcpy(host, source->nick, BUFSIZE);
        mowgli_strlcat(host, "!", BUFSIZE);
        mowgli_strlcat(host, source->user, BUFSIZE);
        mowgli_strlcat(host, "@", BUFSIZE);
        mowgli_strlcat(host, source->host, BUFSIZE);

        MOWGLI_ITER_FOREACH(n, svs_ignore_list.head)
        {
                svsignore = (struct svsignore *)n->data;

                if (!match(svsignore->mask, host))
                        return svsignore;
        }

        return NULL;
}

/*
 * svsignore_delete(struct svsignore *svsignore)
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
void
svsignore_delete(struct svsignore *svsignore)
{
	mowgli_node_t *n;

	n = mowgli_node_find(svsignore, &svs_ignore_list);
	mowgli_node_delete(n, &svs_ignore_list);

	free(svsignore->mask);
	free(svsignore->reason);
	free(svsignore);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
