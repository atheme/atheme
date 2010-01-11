#include "atheme.h"
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <netdb.h>

DECLARE_MODULE_V1
(
    "nickserv/mxcheck", false, _modinit, _moddeinit,
    "1.1",
    "Jamie L. Penman-Smithson <jamie@slacked.org>"
);

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
}

static void check_registration(hook_user_register_check_t *hdata)
{
    char buf[1024];
    const char *user;
    const char *domain;
    int count;

    if (hdata->approved)
        return;

    strlcpy(buf, hdata->email, sizeof buf);
    user = strtok(buf, "@");
    domain = strtok(NULL, "@");
    count = count_mx(domain);

    if (count > 0)
    {
        /* there are MX records for this domain */
        slog(LG_INFO, "REGISTER: mxcheck: %d MX records for %s", count, domain);
    }
    else
    {
        /* no MX records or error */
        struct hostent *host;

        /* attempt to resolve host (fallback to A) */
        if((host = gethostbyname(domain)) == NULL)
        {
            slog(LG_INFO, "REGISTER: mxcheck: no A/MX records for %s - " 
                "REGISTER failed", domain);
            command_fail(hdata->si, fault_noprivs, "Sorry, \2%s\2 does not exist, "
                "I can't send mail there. Please check and try again.", domain);
            hdata->approved = 1;
            return;
        }
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
