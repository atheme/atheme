/*
 * Copyright (c) 2009 Jilles Tjoelker, et al
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Monitors exit of given processes, using kqueue.
 * The kqueue is added to the main poll loop.
 */

#include "atheme.h"
#include <sys/event.h>

DECLARE_MODULE_V1
(
	"operserv/procwatch", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void procwatch_readhandler(connection_t *cptr);

static void os_cmd_procwatch(sourceinfo_t *si, int parc, char *parv[]);

command_t os_procwatch = { "PROCWATCH", "Notifies snoop channel on process exit.",
                        PRIV_ADMIN, 1, os_cmd_procwatch, { .path = "contrib/procwatch" } };

static connection_t *kq_conn;

void _modinit(module_t *m)
{
	int kq;

	kq = kqueue();
	if (kq == -1)
	{
		m->mflags = MODTYPE_FAIL;
		return;
	}
	kq_conn = connection_add("procwatch kqueue", kq, 0, procwatch_readhandler, NULL);

	service_named_bind_command("operserv", &os_procwatch);
}

void _moddeinit()
{
	if (kq_conn != NULL)
		connection_close_soon(kq_conn);
	service_named_unbind_command("operserv", &os_procwatch);
}

static void procwatch_readhandler(connection_t *cptr)
{
	struct kevent ev;

	if (cptr != kq_conn)
	{
		slog(LG_INFO, "procwatch_readhandler(): called with unexpected fd %d", cptr->fd);
		return;
	}

	while (kevent(cptr->fd, NULL, 0, &ev, 1, &(const struct timespec){ 0, 0 }) > 0)
	{
		slog(LG_INFO, "PROCWATCH: %ld exited with status %x",
				(long)ev.ident, (unsigned)ev.data);
	}
}

static void os_cmd_procwatch(sourceinfo_t *si, int parc, char *parv[])
{
	long v;
	char *end;
	struct kevent ev;

	if (parc == 0)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "PROCWATCH");
		command_fail(si, fault_needmoreparams, _("Syntax: PROCWATCH <pid>"));
		return;
	}

	errno = 0;
	v = strtol(parv[0], &end, 10);
	if (errno != 0 || *end != '\0' || v < 0 || (pid_t)v != v)
	{
		command_fail(si, fault_needmoreparams, STR_INVALID_PARAMS, "PROCWATCH");
		command_fail(si, fault_needmoreparams, _("Syntax: PROCWATCH <pid>"));
		return;
	}
	EV_SET(&ev, v, EVFILT_PROC, EV_ADD | EV_ENABLE, NOTE_EXIT, 0, NULL);
	if (kevent(kq_conn->fd, &ev, 1, NULL, 0, NULL) == -1)
	{
		command_fail(si, fault_toomany, _("Failed to add pid %ld"), v);
		return;
	}
	command_success_nodata(si, "Added pid %ld to list.", v);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
