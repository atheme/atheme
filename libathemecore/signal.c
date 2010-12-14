/*
 * atheme-services: A collection of minimalist IRC services
 * signal.c: Signal-handling routines.
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
#include "uplink.h"
#include "conf.h"
#include "internal.h"
#include <signal.h>
#include <sys/wait.h>

static void childproc_check(void);

static volatile sig_atomic_t got_sighup, got_sigint, got_sigterm, got_sigchld, got_sigusr2;

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
signal_chld_handler(int signum)
{
	got_sigchld = 1;
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
	int n;
	if (me.connected && curr_uplink != NULL &&
		curr_uplink->conn != NULL)
	{
		if (chansvs.nick != NULL)
		{
			n = write(curr_uplink->conn->fd, ":", 1);
			n = write(curr_uplink->conn->fd, chansvs.nick, strlen(chansvs.nick));
			n = write(curr_uplink->conn->fd, " QUIT :Out of memory!\r\n", 23);
		}
		n = write(curr_uplink->conn->fd, "ERROR :Panic! Out of memory.\r\n", 30);
	}
	if (runflags & (RF_LIVE | RF_STARTING))
		n = write(2, "Out of memory!\n", 15);
	abort();
}

void init_signal_handlers(void)
{
	signal_install_handler(SIGHUP, signal_hup_handler);
	signal_install_handler(SIGINT, signal_int_handler);
	signal_install_handler(SIGTERM, signal_term_handler);
	signal_install_handler(SIGPIPE, signal_empty_handler);
	signal_install_handler(SIGCHLD, signal_chld_handler);

	signal_install_handler(SIGUSR1, signal_usr1_handler);
	signal_install_handler(SIGUSR2, signal_usr2_handler);
}

void check_signals(void)
{
	/* rehash */
	if (got_sighup)
	{
		got_sighup = 0;
		slog(LG_INFO, "sighandler(): got SIGHUP, rehashing \2%s\2", config_file);

		wallops(_("Got SIGHUP; reloading \2%s\2."), config_file);

		if (db_save && !readonly)
		{
			slog(LG_INFO, "UPDATE: \2%s\2", "system console");
			wallops(_("Updating database by request of \2%s\2."), "system console");
			db_save(NULL);
		}

		slog(LG_INFO, "REHASH: \2%s\2", "system console");
		wallops(_("Rehashing \2%s\2 by request of \2%s\2."), config_file, "system console");

		/* reload the config, opening other logs besides the core log if needed. */
		if (!conf_rehash())
			wallops(_("REHASH of \2%s\2 failed. Please correct any errors in the file and try again."), config_file);

		return;
	}

	/* usually caused by ^C */
	if (got_sigint && (runflags & RF_LIVE))
	{
		got_sigint = 0;
		wallops(_("Exiting on signal %d."), SIGINT);
		if (chansvs.me != NULL && chansvs.me->me != NULL)
			quit_sts(chansvs.me->me, "caught interrupt");
		me.connected = false;
		slog(LG_INFO, "sighandler(): caught interrupt; exiting...");
		runflags |= RF_SHUTDOWN;
	}
	else if (got_sigint && !(runflags & RF_LIVE))
	{
		got_sigint = 0;
		wallops(_("Got SIGINT; restarting."));

		slog(LG_INFO, "RESTART: \2%s\2", "system console");
		wallops(_("Restarting by request of \2%s\2."), "system console");

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

		slog(LG_INFO, "RESTART: \2%s\2", "system console");
		wallops(_("Restarting by request of \2%s\2."), "system console");

		runflags |= RF_RESTART;
	}

	if (got_sigchld)
	{
		got_sigchld = 0;
		childproc_check();
	}
}

mowgli_list_t childproc_list;

struct childproc
{
	mowgli_node_t node;
	pid_t pid;
	char *desc;
	void (*cb)(pid_t pid, int status, void *data);
	void *data;
};

/* Registers a child process.
 * Will call cb(pid, status, data) after the process terminates
 * where status is the status from waitpid(2).
 */
void childproc_add(pid_t pid, const char *desc, void (*cb)(pid_t pid, int status, void *data), void *data)
{
	struct childproc *p;

	p = smalloc(sizeof(*p));
	p->pid = pid;
	p->desc = sstrdup(desc);
	p->cb = cb;
	p->data = data;
	mowgli_node_add(p, &p->node, &childproc_list);
}

static void childproc_free(struct childproc *p)
{
	free(p->desc);
	free(p);
}

/* Forgets about all child processes with the given callback.
 * Useful if the callback is in a module which is being unloaded.
 */
void childproc_delete_all(void (*cb)(pid_t pid, int status, void *data))
{
	mowgli_node_t *n, *tn;
	struct childproc *p;

	MOWGLI_ITER_FOREACH_SAFE(n, tn, childproc_list.head)
	{
		p = n->data;
		if (p->cb == cb)
		{
			mowgli_node_delete(&p->node, &childproc_list);
			childproc_free(p);
		}
	}
}

static void childproc_check(void)
{
	pid_t pid;
	int status;
	mowgli_node_t *n;
	struct childproc *p;

	while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
	{
		MOWGLI_ITER_FOREACH(n, childproc_list.head)
		{
			p = n->data;
			if (p->pid == pid)
			{
				mowgli_node_delete(&p->node, &childproc_list);
				if (p->cb)
					p->cb(pid, status, p->data);
				childproc_free(p);
				break;
			}
		}
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
