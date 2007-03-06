/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains the signal handling routines.
 *
 * $Id: signal.c 7849 2007-03-06 00:27:39Z pippijn $
 */

#include "atheme.h"
#include "uplink.h"
#include "internal.h"

static int got_sighup, got_sigint, got_sigterm, got_sigusr2;

typedef void (*signal_handler_t) (int);

static signal_handler_t
signal_install_handler_full(int signum, signal_handler_t handler,
			    int *sigtoblock, size_t sigtoblocksize)
{
	struct sigaction action, old_action;
	size_t i;

	action.sa_handler = handler;
	action.sa_flags = SA_RESTART;

	sigemptyset(&action.sa_mask);

	for (i = 0; i < sigtoblocksize; i++)
		sigaddset(&action.sa_mask, sigtoblock[i]);

	if (sigaction(signum, &action, &old_action) == -1)
	{
		slog(LG_DEBUG, "Failed to install signal handler for signal %d", signum);
		return NULL;
	}

	return old_action.sa_handler;
}

/*
 * A version of signal(2) that works more reliably across different
 * platforms.
 *
 * It restarts interrupted system calls, does not reset the handler,
 * and blocks the same signal from within the handler.
 */
static signal_handler_t
signal_install_handler(int signum, signal_handler_t handler)
{
	return signal_install_handler_full(signum, handler, NULL, 0);
}

static void
signal_empty_handler(int signum)
{
	/* do nothing */
}

static void
signal_hup_handler(int signum)
{
	got_sighup = 1;
}

static void
signal_int_handler(int signum)
{
	got_sigint = 1;
}

static void
signal_term_handler(int signum)
{
	got_sigterm = 1;
}

static void
signal_usr2_handler(int signum)
{
	got_sigusr2 = 1;
}

/* XXX */
static void
signal_usr1_handler(int signum)
{
	if (log_file != NULL)
		write(fileno(log_file), "Out of memory!\n", 15);

	if (me.connected && curr_uplink != NULL &&
		curr_uplink->conn != NULL)
	{
		if (chansvs.nick != NULL)
		{
			write(curr_uplink->conn->fd, ":", 1);
			write(curr_uplink->conn->fd, chansvs.nick, strlen(chansvs.nick));
			write(curr_uplink->conn->fd, " QUIT :Out of memory!\r\n", 23);
		}
		write(curr_uplink->conn->fd, "ERROR: Panic! Out of memory.\r\n", 30);
	}
	if (runflags & (RF_LIVE | RF_STARTING))
		write(2, "Out of memory!\n", 15);
	abort();
}

void init_signal_handlers(void)
{
	signal_install_handler(SIGHUP, signal_hup_handler);
	signal_install_handler(SIGINT, signal_int_handler);
	signal_install_handler(SIGTERM, signal_term_handler);
	signal_install_handler(SIGPIPE, signal_empty_handler);

	signal_install_handler(SIGUSR1, signal_usr1_handler);
	signal_install_handler(SIGUSR2, signal_usr2_handler);
}

/* XXX: we can integrate this into the handlers now --nenolod */
void check_signals(void)
{
	/* rehash */
	if (got_sighup)
	{
		got_sighup = 0;
		slog(LG_INFO, "sighandler(): got SIGHUP, rehashing %s", config_file);

		wallops(_("Got SIGHUP; reloading \2%s\2."), config_file);

		snoop("UPDATE: \2%s\2", "system console");
		wallops(_("Updating database by request of \2%s\2."), "system console");
		expire_check(NULL);
		db_save(NULL);

		snoop("REHASH: \2%s\2", "system console");
		wallops(_("Rehashing \2%s\2 by request of \2%s\2."), config_file, "system console");

		if (!conf_rehash())
			wallops(_("REHASH of \2%s\2 failed. Please correct any errors in the file and try again."), config_file);

		/* reopen log file -- jilles */
		if (log_file != NULL)
			fclose(log_file);
		log_file = NULL;
		log_open();

		return;
	}

	/* usually caused by ^C */
	if (got_sigint && (runflags & RF_LIVE))
	{
		got_sigint = 0;
		wallops(_("Exiting on signal %d."), SIGINT);
		if (chansvs.me != NULL && chansvs.me->me != NULL)
			quit_sts(chansvs.me->me, "caught interrupt");
		me.connected = FALSE;
		slog(LG_INFO, "sighandler(): caught interrupt; exiting...");
		runflags |= RF_SHUTDOWN;
	}
	else if (got_sigint && !(runflags & RF_LIVE))
	{
		got_sigint = 0;
		wallops(_("Got SIGINT; restarting."));

		snoop("UPDATE: \2%s\2", "system console");
		wallops(_("Updating database by request of \2%s\2."), "system console");
		expire_check(NULL);
		db_save(NULL);

		snoop("RESTART: \2%s\2", "system console");
		wallops(_("Restarting by request of \2%s\2."), "system console");

		slog(LG_INFO, "sighandler(): restarting...");
		runflags |= RF_RESTART;
	}

	if (got_sigterm)
	{
		got_sigterm = 0;
		wallops(_("Exiting on signal %d."), SIGTERM);
		slog(LG_INFO, "sighandler(): got SIGTERM; exiting...");
		runflags |= RF_SHUTDOWN;
	}

	if (got_sigusr2)
	{
		got_sigusr2 = 0;
		wallops(_("Got SIGUSR2; restarting."));

		snoop("UPDATE: \2%s\2", "system console");
		wallops(_("Updating database by request of \2%s\2."), "system console");
		expire_check(NULL);
		db_save(NULL);

		snoop("RESTART: \2%s\2", "system console");
		wallops(_("Restarting by request of \2%s\2."), "system console");

		slog(LG_INFO, "sighandler(): restarting...");
		runflags |= RF_RESTART;
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
