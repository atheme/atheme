#include "atheme.h"
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <netdb.h>

DECLARE_MODULE_V1
(
    "nickserv/mxcheck_async", false, _modinit, _moddeinit,
    "1.1",
    "Jamie L. Penman-Smithson <jamie@slacked.org>"
);

struct procdata
{
	char name[NICKLEN];
	char email[EMAILLEN];
};

#define MAX_CHILDPROCS 10

static unsigned int proccount;
static struct procdata procdata[MAX_CHILDPROCS];

static void childproc_cb(pid_t pid, int status, void *data);
static void check_registration(hook_user_register_check_t *hdata);

int count_mx (const char *host);

void _modinit(module_t *m)
{
    hook_add_event("user_can_register");
    hook_add_user_can_register(check_registration);
}

void _moddeinit(void)
{
    hook_del_user_can_register(check_registration);
    childproc_delete_all(childproc_cb);
}

static void childproc_cb(pid_t pid, int status, void *data)
{
	struct procdata *pd = data;
	myuser_t *mu;
	const char *domain;

	return_if_fail(proccount > 0);
	proccount--;

	if (!WIFEXITED(status))
		return;

	mu = myuser_find(pd->name);
	if (mu == NULL || strcmp(pd->email, mu->email))
		return;
	domain = strchr(pd->email, '@');
	if (domain == NULL)
		return;
	domain++;

	if (WEXITSTATUS(status) == 1)
	{
		slog(LG_INFO, "REGISTER: mxcheck: no A/MX records for %s - " 
				"REGISTER failed", domain);
		myuser_notice(nicksvs.nick, mu, "Sorry, \2%s\2 does not exist, "
				"I can't send mail there. Please check and try again.", domain);
		object_unref(mu);
	}
	else if (WEXITSTATUS(status) == 0)
	{
        	slog(LG_INFO, "REGISTER: mxcheck: valid MX records for %s", domain);
	}
}

static void check_registration(hook_user_register_check_t *hdata)
{
	char buf[1024];
	const char *user;
	const char *domain;
	int count;
	pid_t pid;
	struct procdata *pd;

	if (hdata->approved)
		return;

	if (proccount >= MAX_CHILDPROCS)
	{
		command_fail(hdata->si, fault_toomany, "Sorry, too many registrations in progress. Try again later.");
		hdata->approved = 1;
		return;
	}
	switch (pid = fork())
	{
		case 0: /* child */
			connection_close_all_fds();
			strlcpy(buf, hdata->email, sizeof buf);
			user = strtok(buf, "@");
			domain = strtok(NULL, "@");
			count = count_mx(domain);

			if (count <= 0)
			{
				/* no MX records or error */
				struct hostent *host;

				/* attempt to resolve host (fallback to A) */
				if((host = gethostbyname(domain)) == NULL)
					_exit(1);
			}
			_exit(0);
			break;
		case -1: /* error */
			slog(LG_ERROR, "fork() failed for check_registration(): %s",
					strerror(errno));
			command_fail(hdata->si, fault_toomany, "Sorry, too many registrations in progress. Try again later.");
			hdata->approved = 1;
			return;
		default: /* parent */
			pd = &procdata[proccount++];
			strlcpy(pd->name, hdata->account, sizeof pd->name);
			strlcpy(pd->email, hdata->email, sizeof pd->email);
			childproc_add(pid, "ns_mxcheck_async", childproc_cb, pd);
			return;
	}
}

int count_mx (const char *host)
{
    u_char nsbuf[4096];
    ns_msg amsg;
    int l;

    l = res_query (host, ns_c_any, ns_t_mx, nsbuf, sizeof (nsbuf));
    if (l < 0) 
    {
        return 0;
    } 
    else 
    {
        ns_initparse (nsbuf, l, &amsg);
        l = ns_msg_count (amsg, ns_s_an);
    }
    
    return l;
}
