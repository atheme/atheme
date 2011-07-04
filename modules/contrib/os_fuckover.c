/* os_fuckover.c - Flood a user off the network by numeric flooding them
 * jdhore1@gmail.com
 */
/* XXX: Eventually make this track nick changes and possibly preserve
 * between quits...or not */

#include "atheme.h"
#include "uplink.h"		/* sts() */


DECLARE_MODULE_V1
(
	"contrib/os_fuckover", false, _modinit, _moddeinit,
	"os_fuckover.c",
	"jdhore1@gmail.com"
);

static void os_cmd_fuckover(sourceinfo_t *si, int parc, char *parv[]);
static void do_fuckover(user_t *u);

command_t os_fuckover = { "FUCKOVER", N_("Flood a user off the network by numeric flooding them."),
                        PRIV_ADMIN, 1, os_cmd_fuckover, { .path = "" } };

void _modinit(module_t *m)
{
	TAINT_ON("Using os_fuckover", "Use of os_fuckover is unsupported and not recommend. Use only at your own risk.");
	service_named_bind_command("operserv", &os_fuckover);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("operserv", &os_fuckover);
}

static void os_cmd_fuckover(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *target;

	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FUCKOVER");
		command_fail(si, fault_needmoreparams, _("Usage: FUCKOVER <user>"));
		return;
	}

	if(!(target = user_find_named(parv[0])))
        {
                command_fail(si, fault_nosuch_target, "\2%s\2 is not on the network.", parv[0]);
                return;
        }

	/* Don't allow fucking over a oper. It's not cool. It's also bad English but... */
	if (UF_IRCOP & target->flags)
	{
		command_fail(si, fault_noprivs, "\2%s\2 is an oper, you can not FUCKOVER them.", parv[0]);
		return;
	}

	logcommand(si, CMDLOG_ADMIN, "FUCKOVER: \2%s\2", parv[0]);
	wallops("%s used FUCKOVER on %s.", get_oper_name(si), parv[0]);
	command_success_nodata(si, "%s is now being fucked over.", parv[0]);
	do_fuckover(target);
}

static void do_fuckover(user_t *u)
{
	int numeric, stop = 0;

	while (!stop)
	{
		for (numeric = 100; numeric < 512; ++numeric)
		{
			if (!u)
			{
				stop = 1;
				break;
			}

			numeric_sts(me.me, numeric, u, ":\n");
		}
	}
}
