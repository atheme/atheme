/*
 * Copyright (c) 2009 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * LDAP authentication.
 */

#include "atheme.h"

#define LDAP_DEPRECATED 1
#include <ldap.h>

DECLARE_MODULE_V1
(
	"auth/ldap", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

mowgli_list_t conf_ldap_table;
struct
{
	char *url;
	char *dnformat;
} ldap_config;
LDAP *ldap_conn;

static void ldap_config_ready(void *unused)
{
	int res;
	char *p;
	static time_t lastwarning;

	if (ldap_conn != NULL)
		ldap_unbind_ext_s(ldap_conn, NULL, NULL);
	ldap_conn = NULL;
	if (ldap_config.url == NULL || ldap_config.dnformat == NULL)
	{
		slog(LG_ERROR, "ldap_config_ready(): ldap {} block missing or invalid");
		return;
	}
	p = strchr(ldap_config.dnformat, '%');
	if (p == NULL || p[1] != 's' || strchr(p + 1, '%'))
	{
		slog(LG_ERROR, "ldap_config_ready(): dnformat must contain exactly one %%s and no other %%");
		return;
	}
	ldap_set_option(NULL, LDAP_OPT_PROTOCOL_VERSION, &(const int){ 3 });
	res = ldap_initialize(&ldap_conn, ldap_config.url);
	if (res != LDAP_SUCCESS)
	{
		slog(LG_ERROR, "ldap_config_ready(): ldap_initialize(%s) failed: %s",
				ldap_config.url, ldap_err2string(res));
		if (CURRTIME > lastwarning + 300)
		{
			slog(LG_INFO, "LDAP:ERROR: \2%s\2", ldap_err2string(res));
			wallops("Problem with LDAP server: %s", ldap_err2string(res));
			lastwarning = CURRTIME;
		}
		return;
	}
	/* short timeouts, because this blocks atheme as a whole */
	ldap_set_option(ldap_conn, LDAP_OPT_TIMEOUT, &(const struct timeval){ 1, 0 });
	ldap_set_option(ldap_conn, LDAP_OPT_NETWORK_TIMEOUT, &(const struct timeval){ 1, 0 });
	ldap_set_option(ldap_conn, LDAP_OPT_DEREF, &(const int){ false });
	ldap_set_option(ldap_conn, LDAP_OPT_REFERRALS, &(const int){ false });
}

static bool ldap_auth_user(myuser_t *mu, const char *password)
{
	int res;
	char dn[512];
	static time_t lastwarning;

	if (ldap_conn == NULL || ldap_config.dnformat == NULL)
		return false;

	if (strchr(entity(mu)->name, ' ') || strchr(entity(mu)->name, ',') ||
			strchr(entity(mu)->name, '/'))
		return false;

	snprintf(dn, sizeof dn, ldap_config.dnformat, entity(mu)->name);
	res = ldap_simple_bind_s(ldap_conn, dn, password);
	switch (res)
	{
		case LDAP_SUCCESS:
			return true;
		case LDAP_INVALID_CREDENTIALS:
			return false;
	}
	slog(LG_INFO, "ldap_auth_user(): ldap_bind_s(%s) failed: %s, reconnecting and retrying",
			dn, ldap_err2string(res));
	ldap_config_ready(NULL);
	if (ldap_conn == NULL)
		return false;
	res = ldap_simple_bind_s(ldap_conn, dn, password);
	switch (res)
	{
		case LDAP_SUCCESS:
			return true;
		case LDAP_INVALID_CREDENTIALS:
			return false;
	}
	slog(LG_ERROR, "ldap_auth_user(): ldap_bind_s(%s) failed: %s",
			dn, ldap_err2string(res));
	if (CURRTIME > lastwarning + 300)
	{
		slog(LG_INFO, "LDAP:ERROR: %s", ldap_err2string(res));
		wallops("Problem with LDAP server: %s", ldap_err2string(res));
		lastwarning = CURRTIME;
	}
	return false;
}

void _modinit(module_t *m)
{
        hook_add_event("config_ready");
        hook_add_config_ready(ldap_config_ready);

	add_subblock_top_conf("LDAP", &conf_ldap_table);
	add_dupstr_conf_item("URL", &conf_ldap_table, 0, &ldap_config.url, NULL);
	add_dupstr_conf_item("DNFORMAT", &conf_ldap_table, 0, &ldap_config.dnformat, NULL);

	auth_user_custom = &ldap_auth_user;

	auth_module_loaded = true;
}

void _moddeinit(void)
{
	auth_user_custom = NULL;

	auth_module_loaded = false;

	if (ldap_conn != NULL)
		ldap_unbind_ext_s(ldap_conn, NULL, NULL);

	hook_del_config_ready(ldap_config_ready);
	del_conf_item("URL", &conf_ldap_table);
	del_conf_item("DNFORMAT", &conf_ldap_table);
	del_top_conf("LDAP");
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
