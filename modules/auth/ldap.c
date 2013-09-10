/*
 * Copyright (c) 2009 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * LDAP authentication.
 */

/*
 Supports the following options:

 url -- required ldap URL (e.g. ldap://host.domain.com/)
 
 then either:

   dnformat -- basedn to authenticate against.  Use %s to specify where
        to insert the nick

 or

   base -- basedn to begin the search for the matching dn of the user
   attribute -- the attribute to search against to find the nick
   binddn -- distinguished name to bind to for searching (optional)
   bindauth -- password for the distinguished name (optional, must specify if binddn given)

*/

#include "atheme.h"

#include <ldap.h>

DECLARE_MODULE_V1("auth/ldap", false, _modinit, _moddeinit, PACKAGE_STRING, "Atheme Development Group <http://www.atheme.org>");

mowgli_list_t conf_ldap_table;
struct
{
	char *url;
	char *dnformat;
	char *attribute;
	char *base;
	char *binddn;
	char *bindauth;
	bool useDN;
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
	if (ldap_config.url == NULL)
	{
		slog(LG_ERROR, "ldap_config_ready(): ldap {} missing url definition");
		return;
	}
	if ((ldap_config.dnformat == NULL) && ((ldap_config.base == NULL) || (ldap_config.attribute == NULL)))
	{
		slog(LG_ERROR, "ldap_config_ready(): ldap {} block requires dnformat or base & attribute definition");
		return;
	}
	if (ldap_config.binddn != NULL && ldap_config.bindauth == NULL)
	{
		slog(LG_ERROR, "ldap_config_ready(): ldap{} block requires bindauth to be defined if binddn is defined");
		return;
	}

	if (ldap_config.dnformat != NULL)
	{
		ldap_config.useDN = true;
		p = strchr(ldap_config.dnformat, '%');
		if (p == NULL || p[1] != 's' || strchr(p + 1, '%'))
		{
			slog(LG_ERROR, "ldap_config_ready(): dnformat must contain exactly one %%s and no other %%");
			return;
		}
	}
	else
		ldap_config.useDN = false;

	ldap_set_option(NULL, LDAP_OPT_PROTOCOL_VERSION, &(const int)
			{
			3});
	res = ldap_initialize(&ldap_conn, ldap_config.url);
	if (res != LDAP_SUCCESS)
	{
		slog(LG_ERROR, "ldap_config_ready(): ldap_initialize(%s) failed: %s", ldap_config.url, ldap_err2string(res));
		if (CURRTIME > lastwarning + 300)
		{
			slog(LG_INFO, "LDAP:ERROR: \2%s\2", ldap_err2string(res));
			wallops("Problem with LDAP server: %s", ldap_err2string(res));
			lastwarning = CURRTIME;
		}
		return;
	}

	/* short timeouts, because this blocks atheme as a whole */
	ldap_set_option(ldap_conn, LDAP_OPT_TIMEOUT, &(const struct timeval){1, 0});
	ldap_set_option(ldap_conn, LDAP_OPT_NETWORK_TIMEOUT, &(const struct timeval){1, 0});
	ldap_set_option(ldap_conn, LDAP_OPT_DEREF, &(const int){false});
	ldap_set_option(ldap_conn, LDAP_OPT_REFERRALS, &(const int){false});
}

static bool ldap_auth_user(myuser_t *mu, const char *password)
{
	int res;
	struct berval cred;
	LDAPMessage *message, *entry;

	if (ldap_conn == NULL)
	{
		ldap_config_ready(NULL);
	}
	if (ldap_conn == NULL)
	{
		slog(LG_INFO, "ldap_auth_user(): no connection");
		return false;
	}

	if (strchr(entity(mu)->name, ' '))
	{
		slog(LG_INFO, "ldap_auth_user(%s): bad name: found space", entity(mu)->name);
		return false;
	}
	if (strchr(entity(mu)->name, ','))
	{
		slog(LG_INFO, "ldap_auth_user(%s): bad name: found comma", entity(mu)->name);
		return false;
	}
	if (strchr(entity(mu)->name, '/'))
	{
		slog(LG_INFO, "ldap_auth_user(%s): bad name: found /", entity(mu)->name);
		return false;
	}

/* Use DN to find exact match */
	if (ldap_config.useDN)
	{
		char dn[512];
		cred.bv_len = strlen(password);
		/* sstrdup it to remove the const */
		cred.bv_val = sstrdup(password);

		snprintf(dn, sizeof dn, ldap_config.dnformat, entity(mu)->name);
		res = ldap_sasl_bind_s(ldap_conn, dn, LDAP_SASL_SIMPLE, &cred, NULL, NULL, NULL);
		if (res == LDAP_SERVER_DOWN)
		{
			ldap_config_ready(NULL);
			res = ldap_sasl_bind_s(ldap_conn, dn, LDAP_SASL_SIMPLE, &cred, NULL, NULL, NULL);
		}
		if (res == LDAP_SUCCESS)
			return true;
		else if (res == LDAP_INVALID_CREDENTIALS)
		{
			slog(LG_INFO, "ldap_auth_user(%s): ldap auth bind failed: %s", entity(mu)->name, ldap_err2string(res));
			return false;
		}
		slog(LG_INFO, "ldap_auth_user(): ldap_bind_s failed: %s", ldap_err2string(res));
		return false;


/* Use base + attribute search for Auth */
	}
	else
	{
		char what[512];
		char *binddn = NULL;

		cred.bv_len = 0;

		if (ldap_config.binddn != NULL && ldap_config.bindauth != NULL)
		{
			binddn = ldap_config.binddn;
			cred.bv_val = ldap_config.bindauth;
			cred.bv_len = strlen(ldap_config.bindauth);
		}

		res = ldap_sasl_bind_s(ldap_conn, binddn, LDAP_SASL_SIMPLE, &cred, NULL, NULL, NULL);
		if (res == LDAP_SERVER_DOWN)
		{
			ldap_config_ready(NULL);
			res = ldap_sasl_bind_s(ldap_conn, binddn, LDAP_SASL_SIMPLE, &cred, NULL, NULL, NULL);
		}
		if (res != LDAP_SUCCESS)
		{
			slog(LG_INFO, "ldap_auth_user(): ldap_bind_s failed: %s", ldap_err2string(res));
			return false;
		}

		sprintf(what, "%s=%s", ldap_config.attribute, entity(mu)->name);
		if ((res = ldap_search_ext_s(ldap_conn, ldap_config.base, LDAP_SCOPE_SUBTREE, what, NULL, 0, NULL, NULL, NULL, 0, &message)) != LDAP_SUCCESS)
		{
			slog(LG_INFO, "ldap_auth_user(%s): ldap search failed: %s", entity(mu)->name, ldap_err2string(res));
			return false;
		}

		cred.bv_len = strlen(password);
		/* sstrdup it to remove the const */
		cred.bv_val = sstrdup(password);

		for (entry = ldap_first_message(ldap_conn, message); entry && ldap_msgtype(entry) == LDAP_RES_SEARCH_ENTRY; entry = ldap_next_message(ldap_conn, entry))
		{

			res = ldap_sasl_bind_s(ldap_conn, ldap_get_dn(ldap_conn, entry), LDAP_SASL_SIMPLE, &cred, NULL, NULL, NULL);
			if (res == LDAP_SUCCESS)
			{
				ldap_msgfree(message);
				return true;
			}
		}

		ldap_msgfree(message);
	}

	slog(LG_INFO, "ldap_auth_user(%s): ldap auth bind failed: %s", entity(mu)->name, ldap_err2string(res));
	return false;
}

void _modinit(module_t * m)
{
	hook_add_event("config_ready");
	hook_add_config_ready(ldap_config_ready);

	add_subblock_top_conf("LDAP", &conf_ldap_table);
	add_dupstr_conf_item("URL", &conf_ldap_table, 0, &ldap_config.url, NULL);
	add_dupstr_conf_item("DNFORMAT", &conf_ldap_table, 0, &ldap_config.dnformat, NULL);
	add_dupstr_conf_item("BASE", &conf_ldap_table, 0, &ldap_config.base, NULL);
	add_dupstr_conf_item("ATTRIBUTE", &conf_ldap_table, 0, &ldap_config.attribute, NULL);
	add_dupstr_conf_item("BINDDN", &conf_ldap_table, 0, &ldap_config.binddn, NULL);
	add_dupstr_conf_item("BINDAUTH", &conf_ldap_table, 0, &ldap_config.bindauth, NULL);

	auth_user_custom = &ldap_auth_user;

	auth_module_loaded = true;
}

void _moddeinit(module_unload_intent_t intent)
{
	auth_user_custom = NULL;

	auth_module_loaded = false;

	if (ldap_conn != NULL)
		ldap_unbind_ext_s(ldap_conn, NULL, NULL);

	hook_del_config_ready(ldap_config_ready);
	del_conf_item("URL", &conf_ldap_table);
	del_conf_item("DNFORMAT", &conf_ldap_table);
	del_conf_item("BASE", &conf_ldap_table);
	del_conf_item("ATTRIBUTE", &conf_ldap_table);
	del_conf_item("BINDDN", &conf_ldap_table);
	del_conf_item("BINDAUTH", &conf_ldap_table);
	del_top_conf("LDAP");
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
