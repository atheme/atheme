/*
 * Copyright (c) 2012 William Pitcock <nenolod@dereferenced.org>.
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

DECLARE_MODULE_V1("security/cmdperm", false, _modinit, _moddeinit, PACKAGE_VERSION, "Atheme Development Group <http://www.atheme.org>");

static bool (*parent_command_authorize)(service_t *svs, sourceinfo_t *si, command_t *c, const char *userlevel) = NULL;

static bool cmdperm_command_authorize(service_t *svs, sourceinfo_t *si, command_t *c, const char *userlevel)
{
	char permbuf[BUFSIZE], *cp;

	snprintf(permbuf, sizeof permbuf, "command:%s:%s", svs->internal_name, c->name);
	for (cp = permbuf; *cp != '\0'; cp++)
		*cp = ToLower(*cp);

	if (!has_priv(si, permbuf))
	{
		logaudit_denycmd(si, c, permbuf);
		return false;
	}

	return parent_command_authorize(svs, si, c, userlevel);
}

void _modinit(module_t *m)
{
	parent_command_authorize = command_authorize;
	command_authorize = cmdperm_command_authorize;
}

void _moddeinit(module_unload_intent_t intent)
{
	command_authorize = parent_command_authorize;
}
