/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains the signal handling routines.
 *
 * $Id: signal.c 5846 2006-07-10 16:01:44Z jilles $
 */

#include "atheme.h"

#ifdef _WIN32
#define SIGHUP 0
#define SIGUSR1 0
#define SIGUSR2 0
#endif

static int got_sighup, got_sigint, got_sigterm, got_sigusr2;

void sighandler(int signum)
{
	if (signum == SIGHUP)
		got_sighup = 1;
	else if (signum == SIGINT)
		got_sigint = 1;
	else if (signum == SIGTERM)
		got_sigterm = 1;
	else if (signum == SIGUSR1)
	{
		/* XXX */
		if (log_file != NULL)
			write(fileno(log_file), "Out of memory!\n", 15);
		if (me.connected && curr_uplink != NULL &&
				curr_uplink->conn != NULL)
		{
			write(curr_uplink->conn->fd, ":", 1);
			write(curr_uplink->conn->fd, chansvs.nick, strlen(chansvs.nick));
			write(curr_uplink->conn->fd, " QUIT :Out of memory!\r\n", 23);
			write(curr_uplink->conn->fd, "ERROR :Panic! Out of memory.\r\n", 30);
		}
		if (runflags & (RF_LIVE | RF_STARTING))
			write(2, "Out of memory!\n", 15);
		abort();
	}
	else if (signum == SIGUSR2)
		got_sigusr2 = 1;
}

void check_signals(void)
{
	/* rehash */
	if (got_sighup)
	{
		got_sighup = 0;
		slog(LG_INFO, "sighandler(): got SIGHUP, rehashing %s", config_file);

		wallops("Got SIGHUP; reloading \2%s\2.", config_file);

		snoop("UPDATE: \2%s\2", "system console");
		wallops("Updating database by request of \2%s\2.", "system console");
		expire_check(NULL);
		db_save(NULL);

		snoop("REHASH: \2%s\2", "system console");
		wallops("Rehashing \2%s\2 by request of \2%s\2.", config_file, "system console");

		if (!conf_rehash())
			wallops("REHASH of \2%s\2 failed. Please corrrect any errors in the " "file and try again.", config_file);

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
		wallops("Exiting on signal %d.", SIGINT);
		if (chansvs.me != NULL && chansvs.me->me != NULL)
			quit_sts(chansvs.me->me, "caught interrupt");
		me.connected = FALSE;
		slog(LG_INFO, "sighandler(): caught interrupt; exiting...");
		runflags |= RF_SHUTDOWN;
	}
	else if (got_sigint && !(runflags & RF_LIVE))
	{
		got_sigint = 0;
		wallops("Got SIGINT; restarting.");

		snoop("UPDATE: \2%s\2", "system console");
		wallops("Updating database by request of \2%s\2.", "system console");
		expire_check(NULL);
		db_save(NULL);

		snoop("RESTART: \2%s\2", "system console");
		wallops("Restarting by request of \2%s\2.", "system console");

		slog(LG_INFO, "sighandler(): restarting...");
		runflags |= RF_RESTART;
	}

	if (got_sigterm)
	{
		got_sigterm = 0;
		wallops("Exiting on signal %d.", SIGTERM);
		slog(LG_INFO, "sighandler(): got SIGTERM; exiting...");
		runflags |= RF_SHUTDOWN;
	}

#if 0
	if (signum == SIGUSR1)
	{
		wallops("Panic! Out of memory.");
		if (chansvs.me != NULL && chansvs.me->me != NULL)
			quit_sts(chansvs.me->me, "out of memory!");
		me.connected = FALSE;
		slog(LG_INFO, "sighandler(): out of memory; exiting");
		runflags |= RF_SHUTDOWN;
	}
#endif
	if (got_sigusr2)
	{
		got_sigusr2 = 0;
		wallops("Got SIGUSR2; restarting.");

		snoop("UPDATE: \2%s\2", "system console");
		wallops("Updating database by request of \2%s\2.", "system console");
		expire_check(NULL);
		db_save(NULL);

		snoop("RESTART: \2%s\2", "system console");
		wallops("Restarting by request of \2%s\2.", "system console");

		slog(LG_INFO, "sighandler(): restarting...");
		runflags |= RF_RESTART;
	}
}
