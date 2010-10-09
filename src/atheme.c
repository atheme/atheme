/*
 * atheme-services: A collection of minimalist IRC services   
 * atheme.c: Initialization and startup of the services system
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
#include "conf.h"
#include "uplink.h"
#include "pmodule.h" /* pcommand_init */
#include "internal.h"
#include "datastream.h"
#include "authcookie.h"
#include <sys/resource.h>

chansvs_t chansvs;
nicksvs_t nicksvs;

mowgli_list_t taint_list = { NULL, NULL, 0 };

me_t me;
struct cnt cnt;

/* XXX */
claro_state_t claro_state;
int runflags;

char *config_file;
char *log_path;
bool cold_start = false;
bool readonly = false;

void (*db_save) (void *arg) = NULL;
void (*db_load) (void) = NULL;

/* *INDENT-OFF* */
static void print_help(void)
{
	printf("usage: atheme [-dhnvr] [-c conf] [-l logfile] [-p pidfile]\n\n"
	       "-c <file>    Specify the config file\n"
	       "-d           Start in debugging mode\n"
	       "-h           Print this message and exit\n"
	       "-r	     Start in read-only mode\n"
	       "-l <file>    Specify the log file\n"
	       "-n           Don't fork into the background (log screen + log file)\n"
	       "-p <file>    Specify the pid file (will be overwritten)\n"
	       "-v           Print version information and exit\n");
}
/* *INDENT-ON* */

static void print_version(void)
{
	int i;

	printf("Atheme IRC Services (%s), build-id %s\n", PACKAGE_STRING, revision);

	for (i = 0; infotext[i] != NULL; i++)
		printf("%s\n", infotext[i]);
}

static void rng_reseed(void *unused)
{
	(void)unused;
	arc4random_addrandom((uint8_t *)&cnt, sizeof cnt);
}

static void process_mowgli_log(const char *line)
{
	slog(LG_INFO, "%s", line);
}

int main(int argc, char *argv[])
{
	bool have_conf = false;
	bool have_log = false;
	char buf[32];
	int i, pid, r;
	FILE *pid_file;
	const char *pidfilename = RUNDIR "/atheme.pid";
#ifdef HAVE_GETRLIMIT
	struct rlimit rlim;
#endif
	curr_uplink = NULL;

	mowgli_init();

	/* Prepare gettext */
#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	textdomain(PACKAGE_NAME);
#endif

	/* change to our local directory */
	if (chdir(PREFIX) < 0)
	{
		perror(PREFIX);
		return 20;
	}

#ifdef HAVE_GETRLIMIT
	/* it appears certian systems *ahem*linux*ahem*
	 * don't dump cores by default, so we do this here.
	 */
	if (!getrlimit(RLIMIT_CORE, &rlim))
	{
		rlim.rlim_cur = rlim.rlim_max;
		setrlimit(RLIMIT_CORE, &rlim);
	}
#endif
	
	/* do command-line options */
	while ((r = getopt(argc, argv, "c:dhrl:np:v")) != -1)
	{
		switch (r)
		{
		  case 'c':
			  config_file = sstrdup(optarg);
			  have_conf = true;
			  break;
		  case 'd':
			  log_force = true;
			  break;
		  case 'h':
			  print_help();
			  exit(EXIT_SUCCESS);
			  break;
		  case 'r':
			  readonly = true;
			  break;
		  case 'l':
			  log_path = sstrdup(optarg);
			  have_log = true;
			  break;
		  case 'n':
			  runflags |= RF_LIVE;
			  break;
		  case 'p':
			  pidfilename = optarg;
			  break;
		  case 'v':
			  print_version();
			  exit(EXIT_SUCCESS);
			  break;
		  default:
			  printf("usage: atheme [-dhnvr] [-c conf] [-l logfile] [-p pidfile]\n");
			  exit(EXIT_SUCCESS);
			  break;
		}
	}

	if (!have_conf)
		config_file = sstrdup(SYSCONFDIR "/atheme.conf");

	if (!have_log)
		log_path = sstrdup(LOGDIR "/atheme.log");

	cold_start = true;

	runflags |= RF_STARTING;

	me.kline_id = 0;
	me.start = time(NULL);
	CURRTIME = me.start;
	srand(arc4random());
	me.execname = argv[0];

	/* set signal handlers */
	init_signal_handlers();

	/* open log */
	log_open();
	mowgli_log_set_cb(process_mowgli_log);

	slog(LG_INFO, "%s is starting up...", PACKAGE_STRING);

	/* check for pid file */
	if ((pid_file = fopen(pidfilename, "r")))
	{
		if (fgets(buf, 32, pid_file))
		{
			pid = atoi(buf);

			if (!kill(pid, 0))
			{
				fprintf(stderr, "atheme: daemon is already running\n");
				exit(EXIT_FAILURE);
			}
		}

		fclose(pid_file);
	}

#if HAVE_UMASK
	/* file creation mask */
	umask(077);
#endif

        event_init();
	strshare_init();
        hooks_init();
        init_netio();
        init_socket_queues();
	db_init();

	translation_init();
#ifdef ENABLE_NLS
	language_init();
#endif
	init_nodes();
	init_confprocess();
	init_newconf();
	servtree_init();

	modules_init();
	pcommand_init();

	conf_init();
	if (!conf_parse(config_file))
	{
		slog(LG_ERROR, "Error loading config file %s, aborting",
				config_file);
		exit(EXIT_FAILURE);
	}

	if (config_options.languagefile)
	{
		slog(LG_DEBUG, "Using language: %s", config_options.languagefile);
		if (!conf_parse(config_options.languagefile))
			slog(LG_INFO, "Error loading language file %s, continuing",
					config_options.languagefile);
	}

	authcookie_init();
	common_ctcp_init();

	if (!backend_loaded && authservice_loaded)
	{
		slog(LG_ERROR, "atheme: no backend modules loaded, see your configuration file.");
		exit(EXIT_FAILURE);
	}

	/* we've done the critical startup steps now */
	cold_start = false;

	/* load our db */
	if (db_load)
		db_load();
	else if (backend_loaded)
	{
		slog(LG_ERROR, "atheme: backend module does not provide db_load()!");
		exit(EXIT_FAILURE);
	}
	db_check();

#ifdef HAVE_FORK
	/* fork into the background */
	if (!(runflags & RF_LIVE))
	{
		close(0);
		if (open("/dev/null", O_RDWR) != 0)
		{
			slog(LG_ERROR, "unable to open /dev/null??");
			exit(EXIT_FAILURE);
		}
		if ((i = fork()) < 0)
		{
			slog(LG_ERROR, "can't fork into the background");
			exit(EXIT_FAILURE);
		}

		/* parent */
		else if (i != 0)
		{
			slog(LG_INFO, "pid %d", i);
			slog(LG_INFO, "running in background mode from %s", PREFIX);
			exit(EXIT_SUCCESS);
		}

		/* parent is gone, just us now */
		if (setsid() < 0)
		{
			slog(LG_ERROR, "unable to create new session: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}
		dup2(0, 1);
		dup2(0, 2);
	}
	else
	{
		slog(LG_INFO, "pid %d", getpid());
		slog(LG_INFO, "running in foreground mode from %s", PREFIX);
	}
#else
	slog(LG_INFO, "running in foreground mode from %s", PREFIX);
#endif

#ifdef HAVE_GETPID
	/* write pid */
	if ((pid_file = fopen(pidfilename, "w")))
	{
		fprintf(pid_file, "%d\n", getpid());
		fclose(pid_file);
	}
	else
	{
		fprintf(stderr, "atheme: unable to write pid file\n");
		exit(EXIT_FAILURE);
	}
#endif
	/* no longer starting */
	runflags &= ~RF_STARTING;

	/* we probably have a few open already... */
	me.maxfd = 3;

	/* DB commit interval is configurable */
	if (db_save && !readonly)
		event_add("db_save", db_save, NULL, config_options.commit_interval);

	/* check expires every hour */
	event_add("expire_check", expire_check, NULL, 3600);

	/* check k/x/q line expires every minute */
	event_add("kline_expire", kline_expire, NULL, 60);
	event_add("xline_expire", xline_expire, NULL, 60);
	event_add("qline_expire", qline_expire, NULL, 60);

	/* check authcookie expires every ten minutes */
	event_add("authcookie_expire", authcookie_expire, NULL, 600);

	/* reseed rng a little every five minutes */
	event_add("rng_reseed", rng_reseed, NULL, 293);

	me.connected = false;
	uplink_connect();

	/* main loop */
	io_loop();

	/* we're shutting down */
	hook_call_shutdown();

	if (db_save && !readonly)
		db_save(NULL);

	remove(pidfilename);
	errno = 0;
	if (curr_uplink != NULL && curr_uplink->conn != NULL)
		sendq_flush(curr_uplink->conn);
	connection_close_all();

	me.connected = false;

	/* should we restart? */
	if (runflags & RF_RESTART)
	{
		slog(LG_INFO, "main(): restarting");

#ifdef HAVE_EXECVE
		execv(BINDIR "/atheme-services", argv);
#endif
	}

	slog(LG_INFO, "main(): shutting down");

	log_shutdown();

	return 0;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
