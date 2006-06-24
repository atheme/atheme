/*
 * Copyright (c) 2005-2006 Atheme Development Group.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 * $Id: atheme.c 5546 2006-06-24 23:08:30Z jilles $
 */

#include "atheme.h"

chansvs_t chansvs;
globsvs_t globsvs;
opersvs_t opersvs;
memosvs_t memosvs;
helpsvs_t helpsvs;
nicksvs_t nicksvs;
usersvs_t usersvs;
saslsvs_t saslsvs;

me_t me;
cnt_t cnt;

char *config_file;
char *log_path;
boolean_t cold_start = FALSE;

#ifndef _WIN32
extern char **environ;
#endif

/* *INDENT-OFF* */
static void print_help(void)
{
	printf("usage: atheme [-dhnv] [-c config] [-p pidfile]\n\n"
	       "-c <file>    Specify the config file\n"
	       "-d           Start in debugging mode\n"
	       "-h           Print this message and exit\n"
	       "-l <file>    Specify the log file\n"
	       "-n           Don't fork into the background (log screen + log file)\n"
	       "-p <file>    Specify the pid file (will be overwritten)\n"
	       "-v           Print version information and exit\n");
}

static void print_version(void)
{
	printf("Atheme IRC Services (atheme-%s)\n"
	       "Compiled %s, build-id %s, build %s\n\n"
	       "Copyright (c) 2005-2006 Atheme Development Group\n"
	       "Rights to this code are documented in doc/LICENSE.\n",
	       version, creation, revision, generation);
}
/* *INDENT-ON* */

int main(int argc, char *argv[])
{
	boolean_t have_conf = FALSE;
	boolean_t have_log = FALSE;
	char buf[32];
	int i, pid, r;
	FILE *pid_file;
	char *pidfilename = "var/atheme.pid";
#ifndef _WIN32
	struct rlimit rlim;
#endif
	curr_uplink = NULL;

	/* change to our local directory */
	if (chdir(PREFIX) < 0)
	{
		perror(PREFIX);
		return 20;
	}

#ifndef _WIN32
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
	while ((r = getopt(argc, argv, "c:dhl:np:v")) != -1)
	{
		switch (r)
		{
		  case 'c':
			  config_file = sstrdup(optarg);
			  have_conf = TRUE;
			  break;
		  case 'd':
			  me.loglevel |= LG_DEBUG;
			  break;
		  case 'h':
			  print_help();
			  exit(EXIT_SUCCESS);
			  break;
		  case 'l':
			  log_path = sstrdup(optarg);
			  have_log = TRUE;
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
			  printf("usage: atheme [-dhnv] [-c conf] [-l logfile] [-p pidfile]\n");
			  exit(EXIT_SUCCESS);
			  break;
		}
	}

	if (have_conf == FALSE)
		config_file = sstrdup("etc/atheme.conf");

	if (have_log == FALSE)
		log_path = sstrdup("var/atheme.log");

	cold_start = TRUE;

	runflags |= RF_STARTING;

	me.start = time(NULL);
	CURRTIME = me.start;
	me.execname = argv[0];

	/* set signal handlers, first part */
	signal(SIGFPE, sighandler);
	signal(SIGILL, sighandler);
#ifndef _WIN32
	signal(SIGPIPE, SIG_IGN);
	signal(SIGQUIT, sighandler);
	/* on daemonizing, we may get a SIGHUP if the parent exits,
	 * and it's a controlling process -- jilles */
	signal(SIGHUP, SIG_IGN);
	signal(SIGTRAP, sighandler);
	signal(SIGIOT, sighandler);
	signal(SIGALRM, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGWINCH, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGUSR1, sighandler);
#endif

	/* open log */
	log_open();
	if (log_file == NULL)
		fprintf(stderr, "atheme: unable to open log file!\n");

	/* since me.loglevel isn't there until after the
	 * config routines run, we set the default here
	 */
	me.loglevel |= LG_ERROR;

	printf("atheme: version atheme-%s\n", version);

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

	libclaro_init(slog);

	init_nodes();
	init_newconf();
	servtree_init();
	init_ircpacket();

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

	if (!backend_loaded)
	{
		fprintf(stderr, "atheme: no backend modules loaded, see your configuration file.\n");
		exit(EXIT_FAILURE);
	}

	/* check our config file */
	if (!conf_check())
		exit(EXIT_FAILURE);

	/* we've done the critical startup steps now */
	cold_start = FALSE;

	/* load our db */
	if (db_load)
		db_load();
	else
	{
		/* XXX: We should have bailed by now! --nenolod */
		fprintf(stderr, "atheme: no backend modules loaded, see your configuration file.\n");
		exit(EXIT_FAILURE);
	}
	db_check();

#ifndef _WIN32
	/* fork into the background */
	if (!(runflags & RF_LIVE))
	{
		close(0);
		if (open("/dev/null", O_RDWR) != 0)
		{
			fprintf(stderr, "atheme: unable to open /dev/null??\n");
			exit(EXIT_FAILURE);
		}
		if ((i = fork()) < 0)
		{
			fprintf(stderr, "atheme: can't fork into the background\n");
			exit(EXIT_FAILURE);
		}

		/* parent */
		else if (i != 0)
		{
			printf("atheme: pid %d\n", i);
			printf("atheme: running in background mode from %s\n", PREFIX);
			exit(EXIT_SUCCESS);
		}

		/* parent is gone, just us now */
		if (setsid() < 0)
		{
			fprintf(stderr, "atheme: unable to create new session\n");
			exit(EXIT_FAILURE);
		}
		dup2(0, 1);
		dup2(0, 2);
	}
	else
	{
		printf("atheme: pid %d\n", getpid());
		printf("atheme: running in foreground mode from %s\n", PREFIX);
	}
#else
	printf("atheme: running in foreground mode from %s\n", PREFIX);
#endif

#ifndef _WIN32
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
	/* rest of signal handlers now we're started more fully -- jilles */
	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);
#ifndef _WIN32
	signal(SIGHUP, sighandler);
	signal(SIGUSR2, sighandler);
#endif

	/* no longer starting */
	runflags &= ~RF_STARTING;

	/* we probably have a few open already... */
	me.maxfd = 3;

	/* DB commit interval is configurable */
	event_add("db_save", db_save, NULL, config_options.commit_interval);

	/* check expires every hour */
	event_add("expire_check", expire_check, NULL, 3600);

	/* check kline expires every minute */
	event_add("kline_expire", kline_expire, NULL, 60);

	/* check authcookie expires every ten minutes */
	event_add("authcookie_expire", authcookie_expire, NULL, 600);

	uplink_connect();
	me.connected = FALSE;

	/* main loop */
	io_loop();

	/* we're shutting down */
	db_save(NULL);
	if (chansvs.me != NULL && chansvs.me->me != NULL)
		quit_sts(chansvs.me->me, "shutting down");

	remove(pidfilename);
	errno = 0;
	sendq_flush(curr_uplink->conn);
	connection_close(curr_uplink->conn);

	me.connected = FALSE;

	/* should we restart? */
	if (runflags & RF_RESTART)
	{
		slog(LG_INFO, "main(): restarting");

#ifndef _WIN32
		execve("bin/atheme", argv, environ);
#endif
	}

	slog(LG_INFO, "main(): shutting down");

	if (log_file != NULL)
		fclose(log_file);

	return 0;
}
