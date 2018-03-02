/*
 * Copyright (c) 2011 Alexandria Wolcott
 * Released under the same terms as Atheme itself.
 *
 * Netsplit monitor
 */

#include "atheme.h"

struct netsplit
{
    char *name;
    time_t disconnected_since;
    unsigned int flags;
};

static mowgli_patricia_t *ss_netsplit_cmds = NULL;
static mowgli_patricia_t *splitlist = NULL;
static mowgli_heap_t *split_heap = NULL;

static void
netsplit_delete_serv(struct netsplit *s)
{
    mowgli_patricia_delete(splitlist, s->name);
    free(s->name);

    mowgli_heap_free(split_heap, s);
}

static void
netsplit_server_add(struct server *s)
{
    struct netsplit *serv = mowgli_patricia_retrieve(splitlist, s->name);
    if (serv != NULL)
    {
        netsplit_delete_serv(serv);
    }
}

static void
netsplit_server_delete(hook_server_delete_t *serv)
{
    struct netsplit *s;

    s = mowgli_heap_alloc(split_heap);
    s->name = sstrdup(serv->s->name);
    s->disconnected_since = CURRTIME;
    s->flags = serv->s->flags;
    mowgli_patricia_add(splitlist, s->name, s);
}

static void
ss_cmd_netsplit(struct sourceinfo * si, int parc, char *parv[])
{
    struct command *c;
    char *cmd = parv[0];

    if (!cmd)
    {
        command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "NETSPLIT");
        command_fail(si, fault_needmoreparams,
                _("Syntax: NETSPLIT [LIST|REMOVE] [parameters]"));
        return;
    }

    c = command_find(ss_netsplit_cmds, cmd);
    if (c == NULL)
    {
        command_fail(si, fault_badparams,
                _("Invalid command. Use \2/%s%s help\2 for a command listing."),
                (ircd->uses_rcommand == false) ? "msg " : "", si->service->disp);
        return;
    }

    command_exec(si->service, si, c, parc - 1, parv + 1);
}

static void
ss_cmd_netsplit_list(struct sourceinfo * si, int parc, char *parv[])
{
    struct netsplit *s;
    mowgli_patricia_iteration_state_t state;
    int i = 0;

    MOWGLI_PATRICIA_FOREACH(s, &state, splitlist)
    {
        i++;
        command_success_nodata(si, _("%d: %s [Split %s ago]"), i, s->name, time_ago(s->disconnected_since));
    }
    command_success_nodata(si, _("End of netsplit list."));
}

static void
ss_cmd_netsplit_remove(struct sourceinfo * si, int parc, char *parv[])
{
    char *name = parv[0];
    struct netsplit *s;

    if (!name)
    {
        command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "NETSPLIT REMOVE");
        command_fail(si, fault_needmoreparams,
                _("Syntax: NETSPLIT REMOVE <server>"));
        return;
    }

    s = mowgli_patricia_retrieve(splitlist, name);

    if (s != NULL)
    {
        netsplit_delete_serv(s);
        command_success_nodata(si, _("%s removed from the netsplit list."), name);
    }
    else
        command_fail(si, fault_nosuch_target, _("The server \2%s\2 does is not a split server."), name);
}

static struct command ss_netsplit = { "NETSPLIT", N_("Monitor network splits."), PRIV_SERVER_AUSPEX, 2, ss_cmd_netsplit, {.path = "statserv/netsplit"} };
static struct command ss_netsplit_list = { "LIST", N_("List currently split servers."), PRIV_SERVER_AUSPEX, 1, ss_cmd_netsplit_list, {.path = ""} };
static struct command ss_netsplit_remove = { "REMOVE", N_("Remove a server from the netsplit list."), PRIV_JUPE, 2, ss_cmd_netsplit_remove, {.path = ""} };

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
    service_named_bind_command("statserv", &ss_netsplit);

    ss_netsplit_cmds = mowgli_patricia_create(strcasecanon);

    command_add(&ss_netsplit_list, ss_netsplit_cmds);
    command_add(&ss_netsplit_remove, ss_netsplit_cmds);

    hook_add_event("server_add");
    hook_add_event("server_delete");
    hook_add_server_add(netsplit_server_add);
    hook_add_server_delete(netsplit_server_delete);

    split_heap = mowgli_heap_create(sizeof(struct netsplit), 30, BH_NOW);

    if (split_heap == NULL)
    {
        slog(LG_INFO, "statserv/netsplit _modinit(): block allocator failure.");
        exit(EXIT_FAILURE);
    }

    splitlist = mowgli_patricia_create(irccasecanon);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
    mowgli_patricia_iteration_state_t state;
    struct netsplit *s;

    MOWGLI_PATRICIA_FOREACH(s, &state, splitlist)
        netsplit_delete_serv(s);

    mowgli_heap_destroy(split_heap);

    service_named_unbind_command("statserv", &ss_netsplit);

    command_delete(&ss_netsplit_list, ss_netsplit_cmds);
    command_delete(&ss_netsplit_remove, ss_netsplit_cmds);

    hook_del_event("server_add");
    hook_del_event("server_delete");
    hook_del_server_add(netsplit_server_add);
    hook_del_server_delete(netsplit_server_delete);

    mowgli_patricia_destroy(ss_netsplit_cmds, NULL, NULL);
    mowgli_patricia_destroy(splitlist, NULL, NULL);
}

SIMPLE_DECLARE_MODULE_V1("statserv/netsplit", MODULE_UNLOAD_CAPABILITY_OK)
