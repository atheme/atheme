#include "atheme.h"
#include "conf.h"
#include "datastream.h"

DECLARE_MODULE_V1
(
	"on_db_save", false, _modinit, _moddeinit,
	"",
	"Atheme Development Group <http://www.atheme.org>"
);

static char *command = NULL;

static void on_db_save(void *unused);

static struct update_command_state {
	connection_t *out, *err;
	pid_t pid;
	int running;
} update_command_proc;

void _modinit(module_t *m)
{
	hook_add_event("db_saved");
	hook_add_db_saved(on_db_save);

	add_dupstr_conf_item("db_update_command", &conf_gi_table, 0, &command, NULL);
}

void _moddeinit(void)
{
	hook_del_db_saved(on_db_save);

	del_conf_item("db_update_command", &conf_gi_table);
}

static void update_command_finished(pid_t pid, int status, void *data)
{
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
		slog(LG_ERROR, "ERROR: Database update command failed with error %d", WEXITSTATUS(status));

	update_command_proc.running = 0;
}

static void update_command_recvq_handler(connection_t *cptr, int err)
{
	char buf[BUFSIZE];
	int count;

	count = recvq_getline(cptr, buf, sizeof(buf) - 1);
	if (count <= 0)
		return;
	if (buf[count-1] == '\n')
		count--;
	if (count == 0)
		buf[count++] = ' ';
	buf[count] = '\0';

	if (err)
	{
		slog(LG_ERROR, "ERROR: database update command said: %s", buf);
	}
	else
		slog(LG_DEBUG, "db update command stdout: %s", buf);
}

static void update_command_stdout_handler(connection_t *cptr)
{
	update_command_recvq_handler(cptr, 0);
}

static void update_command_stderr_handler(connection_t *cptr)
{
	update_command_recvq_handler(cptr, 1);
}

static void on_db_save(void *unused)
{
	int stdout_pipes[2], stderr_pipes[2];
	pid_t pid;
	int errno1;

	if (!command)
		return;

	if (update_command_proc.running)
	{
		slog(LG_ERROR, "ERROR: database update command is still running");
		return;
	}

	if (pipe(stdout_pipes) == -1)
	{
		int err = errno;
		slog(LG_ERROR, "ERROR: Couldn't create pipe for database update command: %s", strerror(err));
		return;
	}

	if (pipe(stderr_pipes) == -1)
	{
		int err = errno;
		slog(LG_ERROR, "ERROR: Couldn't create pipe for database update command: %s", strerror(err));
		close(stdout_pipes[0]);
		close(stdout_pipes[1]);
		return;
	}

	pid = fork();

	switch (pid)
	{
		case -1:
			errno1 = errno;
			slog(LG_ERROR, "Failed to fork for database update command: %s", strerror(errno1));
			return;
		case 0:
			connection_close_all_fds();
			close(stdout_pipes[0]);
			close(stderr_pipes[0]);
			dup2(stdout_pipes[1], 1);
			dup2(stderr_pipes[1], 2);
			close(stdout_pipes[1]);
			close(stderr_pipes[1]);
			execl("/bin/sh", "sh",  "-c", command, NULL);
			write(2, "Failed to exec /bin/sh\n", 23);
			_exit(255);
			return;
		default:
			close(stdout_pipes[1]);
			close(stderr_pipes[1]);
			update_command_proc.out = connection_add("update_command_stdout", stdout_pipes[0], 0, recvq_put, NULL);
			update_command_proc.err = connection_add("update_command_stderr", stderr_pipes[0], 0, recvq_put, NULL);
			update_command_proc.out->recvq_handler = update_command_stdout_handler;
			update_command_proc.err->recvq_handler = update_command_stderr_handler;
			update_command_proc.pid = pid;
			update_command_proc.running = 1;
			childproc_add(pid, "db_update", update_command_finished, NULL);
			break;
	}
}
