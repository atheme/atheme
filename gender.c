/* Original code: https://gist.github.com/alexwilson/95cbb8ad0f7969a6387571e51bc8bbfb
 * Copyright (c) 2017 Alex Wilson <a@ax.gy>
 * New code:
 * Copyright (c) Kufat <kufat@kufat.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Sets and displays gender identity info.
 */

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"

const char* const bannedwords[] = {"apache", "attack", "helicopter"};
int bannedwordcount = sizeof(bannedwords) / sizeof(bannedwords[0]);

static void
inspircd_send_meta(stringref uid, char* gender)
{
    if(PROTOCOL_INSPIRCD == ircd->type)
    {
        slog(LG_VERBOSE, ":%s METADATA %s %s :%s", me.numeric, uid, "gender", gender);
        sts(":%s METADATA %s %s :%s", me.numeric, uid, "gender", gender);
    }
}

static void
user_info_hook(hook_user_req_t *hdata)
{
    metadata_t *md;

    if (md == metadata_find(hdata->mu, "private:gender"))
    {
        command_success_nodata(hdata->si, _("\2%s\2 identifies as: %s"),
                               entity(hdata->mu)->name,
                               md->value);
    }
}

static void
user_identify_hook(user_t *u)
{
    metadata_t *md;
    if (md = metadata_find(u->myuser, "private:gender"))
    {
        inspircd_send_meta(u->uid, md->value);
    }
}

// GENDER <a word or phrase>

static void
ns_cmd_gender(sourceinfo_t *si, int parc, char *parv[])
{
    char *gender = parv[0];
    metadata_t *md;

    if (!gender)
    {
        md = metadata_find(si->smu, "private:gender");

        if (!md)
        {
            command_fail(si, fault_nochange, _("Your gender was already cleared."));
            return;
        }

        metadata_delete(si->smu, "private:gender");
        logcommand(si, CMDLOG_SET, "GENDER:REMOVE");
        command_success_nodata(si, _("Your gender entry has been deleted."));
        inspircd_send_meta(si->su->uid, "");
        return;
    }

    for (int i = 0; i < bannedwordcount; ++i)
    {
        char* found = strcasestr(gender, bannedwords[i]);
        if (found)
        {
            command_fail(si,
                         fault_badparams,
                         _("The word '%s' is on the banned words list for %s."),
                         bannedwords[i],
                         "GENDER");
            logcommand(si, CMDLOG_SET, "GENDER: Troll gender \2%s\2", gender);
            kill_user(si->service->me, si->su, "%s", "User attempted to set a gender containing "
                                                     "a word on the disallow list.");
            return;
        }
    }

    metadata_add(si->smu, "private:gender", gender);
    logcommand(si, CMDLOG_SET, "GENDER: \2%s\2", gender);
    inspircd_send_meta(si->su->uid, gender);
    command_success_nodata(si, _("Your gender is now set to \2%s\2."), gender);
}

static command_t ns_gender = {
        .name           = "GENDER",
        .desc           = N_("Set gender identity info."),
        .access         = AC_AUTHENTICATED,
        .maxparc        = 1,
        .cmd            = &ns_cmd_gender,
        .help           = { .path = "nickserv/gender" },
};

void
_modinit(module_t *m)
{
    user_t* u;
    mowgli_patricia_iteration_state_t state;

    hook_add_user_info(user_info_hook);
    hook_add_user_identify(user_identify_hook);

    MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/main")

            (void) service_named_bind_command("nickserv", &ns_gender);

    MOWGLI_PATRICIA_FOREACH(u, &state, userlist)
    {
        metadata_t *md;
        if (u->myuser && (md = metadata_find(u->myuser, "private:gender")))
        {
            inspircd_send_meta(u->uid, md->value);
        }
    }

}

void
_moddeinit(module_unload_intent_t intent)
{
    hook_del_user_info(user_info_hook);
    hook_del_user_identify(user_identify_hook);

    (void) service_named_unbind_command("nickserv", &ns_gender);
}


DECLARE_MODULE_V1
(
"nickserv/gender", false, _modinit, _moddeinit,
PACKAGE_STRING,
VENDOR_STRING
);