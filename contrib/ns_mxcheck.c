#include "atheme.h"
#include <netinet/in.h>
#include <resolv.h>
#include <netdb.h>

DECLARE_MODULE_V1
(
    "nickserv/mxcheck", FALSE, _modinit, _moddeinit,
    "1.1",
    "Jamie L. Penman-Smithson <jamie@slacked.org>"
);

static void check_registration(void *vptr);
int count_mx (const char *host);

void _modinit(module_t *m)
{
    hook_add_event("user_can_register");
    hook_add_hook("user_can_register", check_registration);
}

void _moddeinit(void)
{
    hook_del_hook("user_can_register", check_registration);
}

static void check_registration(void *vptr)
{
    hook_user_register_check_t *hdata = vptr;

    if (hdata->approved)
        return;

    char buf[1024];
    const char *delim = "@";
    strcpy(buf, hdata->email);
    const char *user = strtok(buf, delim);
    const char *domain = strtok(NULL, delim);

    int count = count_mx(domain);

    if (count > 0)
    {
        // there are MX records for this domain
        snoop("REGISTER: mxcheck: %d MX records for %s", count, domain);
    }
    else
    {
        // no MX records or error
        struct hostent *host;

        // attempt to resolve host (fallback to A)
        if((host = gethostbyname(domain)) == NULL)
        {
            snoop("REGISTER: mxcheck: no A/MX records for %s - " 
                "REGISTER failed", domain);
            command_fail(hdata->si, fault_noprivs, "Sorry, \2%s\2 does not exist, "
                "I can't send mail there. Please check and try again.");
            hdata->approved = 0;
            return;
        }
    }
}

int count_mx (const char *host)
{
    u_char nsbuf[4096];
    ns_msg msg;
    int l;

    l = res_query (host, ns_c_any, ns_t_mx, nsbuf, sizeof (nsbuf));
    if (l < 0) 
    {
        return 0;
    } 
    else 
    {
        ns_initparse (nsbuf, l, &msg);
        l = ns_msg_count (msg, ns_s_an);
    }
    
    return l;
}
