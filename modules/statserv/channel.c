/*
 * Copyright (c) 2011 Alexandria Wolcott
 * Released under the same terms as Atheme itself.
 *
 * Channel information gatherer for statistics (Useful mostly for XMLRPC)
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1("statserv/channel", false, _modinit, _moddeinit,
        PACKAGE_STRING, "Alexandria Wolcott <alyx@sporksmoo.net>");

static void ss_cmd_channel(sourceinfo_t * si, int parc, char *parv[]);
static void ss_cmd_channel_topic(sourceinfo_t * si, int parc, char *parv[]);
static void ss_cmd_channel_count(sourceinfo_t * si, int parc, char *parv[]);

command_t ss_channel =
{ "CHANNEL", N_("Obtain various information about a channel."), AC_NONE, 2, ss_cmd_channel, {.path = "statserv/channel"} };

command_t ss_channel_topic =
{ "TOPIC", N_("Obtain the topic for a given channel."), AC_NONE, 1, ss_cmd_channel_topic, {.path = ""} };

command_t ss_channel_count =
{ "COUNT", N_("Count the number of channels on the network."), AC_NONE, 1, ss_cmd_channel_count, {.path = ""}};

mowgli_patricia_t *ss_channel_cmds;

void _modinit(module_t * m)
{
    service_named_bind_command("statserv", &ss_channel);

    ss_channel_cmds = mowgli_patricia_create(strcasecanon);

    command_add(&ss_channel_topic, ss_channel_cmds);
    command_add(&ss_channel_count, ss_channel_cmds);
}

void _moddeinit(module_unload_intent_t intent)
{
    service_named_unbind_command("statserv", &ss_channel);

    command_delete(&ss_channel_topic, ss_channel_cmds);
    command_delete(&ss_channel_count, ss_channel_cmds);

    mowgli_patricia_destroy(ss_channel_cmds, NULL, NULL);
}

static void ss_cmd_channel(sourceinfo_t * si, int parc, char *parv[])
{
    command_t *c;
    char *cmd = parv[0];

    if (!cmd)
    {
        command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CHANNEL");
        command_fail(si, fault_needmoreparams,
                _("Syntax: CHANNEL [TOPIC|COUNT] [parameters]"));
        return;
    }

    c = command_find(ss_channel_cmds, cmd);
    if (c == NULL)
    {
        command_fail(si, fault_badparams,
                _("Invalid command. Use \2/%s%s help\2 for a command listing."),
                (ircd->uses_rcommand == false) ? "msg " : "", si->service->disp);
        return;
    }

    command_exec(si->service, si, c, parc - 1, parv + 1);
}

static void ss_cmd_channel_topic(sourceinfo_t * si, int parc, char *parv[])
{
    char *chan = parv[0];
    channel_t *c;

    if (!chan)
    {
        command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CHANNEL TOPIC");
        command_fail(si, fault_needmoreparams, _("Syntax: CHANNEL TOPIC <#channel>"));
        return;
    }

    if (!(c = channel_find(chan)))
    {
        command_fail(si, fault_nosuch_target, _("The channel \2%s\2 does not exist."),
                chan);
        return;
    }

    if (c->modes & CMODE_SEC)
    {
        command_fail(si, fault_noprivs,
                _("You are not authorised to perform this action."));
        return;
    }

    if (c->topic)
        command_success_nodata(si, _("Topic for %s set by %s: %s"), c->name,
                c->topic_setter, c->topic);
    else
        command_success_nodata(si, _("No topic set for %s"), c->name);
}

static void ss_cmd_channel_count(sourceinfo_t * si, int parc, char *parv[])
{
    command_success_nodata(si, "There are %u channels on the network.", mowgli_patricia_size(chanlist));
}
