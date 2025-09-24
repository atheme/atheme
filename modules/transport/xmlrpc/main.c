/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2014 Jilles Tjoelker, et al.
 *
 * New xmlrpc implementation
 */

#include <atheme.h>
#include "xmlrpclib.h"

static struct {
	char *path;
} xmlrpc_config;

static struct connection *current_cptr = NULL; // XXX: Hack: src/xmlrpc.c requires us to do this

static mowgli_list_t *httpd_path_handlers = NULL;

// Configuration
static mowgli_list_t conf_xmlrpc_table;

static char *
dump_buffer(char *buf, int length)
{
	struct httpddata *hd;
	char buf1[300];

	hd = current_cptr->userdata;

	snprintf(buf1, sizeof buf1,
	         "HTTP/1.1 200 OK\r\n"
	         "Server: %s/%s\r\n"
	         "Content-Type: text/xml\r\n"
	         "Content-Length: %d\r\n"
	         "%s"
	         "\r\n",
	         PACKAGE_TARNAME, PACKAGE_VERSION,
	         length,
	         hd->connection_close ? "Connection: close\r\n" : "");

	sendq_add(current_cptr, buf1, strlen(buf1));
	sendq_add(current_cptr, buf, length);

	if (hd->connection_close)
		sendq_add_eof(current_cptr);

	return buf;
}

static void
handle_request(struct connection *cptr, void *requestbuf)
{
	current_cptr = cptr;
	xmlrpc_process(requestbuf, cptr);
	current_cptr = NULL;

	return;
}

static struct path_handler handle_xmlrpc = { NULL, handle_request };

static void
xmlrpc_config_ready(void *vptr)
{
	/* Note: handle_xmlrpc.path may point to freed memory between
	 * reading the config and here.
	 */
	handle_xmlrpc.path = xmlrpc_config.path;

	if (handle_xmlrpc.handler != NULL)
	{
		if (mowgli_node_find(&handle_xmlrpc, httpd_path_handlers))
			return;

		mowgli_node_add(&handle_xmlrpc, mowgli_node_create(), httpd_path_handlers);
	}
	else
		slog(LG_ERROR, "xmlrpc_config_ready(): xmlrpc {} block missing or invalid");
}

static void
xmlrpc_command_fail(struct sourceinfo *si, enum cmd_faultcode code, const char *message)
{
	struct connection *cptr;
	struct httpddata *hd;
	char *newmessage;

	cptr = si->connection;
	hd = cptr->userdata;
	if (hd->sent_reply)
		return;
	newmessage = xmlrpc_normalizeBuffer(message);
	xmlrpc_generic_error(code, newmessage);
	sfree(newmessage);
	hd->sent_reply = true;
}

static void
xmlrpc_command_success_nodata(struct sourceinfo *si, const char *message)
{
	struct connection *cptr;
	struct httpddata *hd;
	char *newmessage;
	char *p;

	newmessage = xmlrpc_normalizeBuffer(message);

	cptr = si->connection;
	hd = cptr->userdata;
	if (hd->sent_reply)
	{
		sfree(newmessage);
		return;
	}
	if (hd->replybuf != NULL)
	{
		hd->replybuf = srealloc(hd->replybuf, strlen(hd->replybuf) + strlen(newmessage) + 2);
		p = hd->replybuf + strlen(hd->replybuf);
		*p++ = '\n';
	}
	else
	{
		hd->replybuf = smalloc(strlen(newmessage) + 1);
		p = hd->replybuf;
	}
        strcpy(p, newmessage);
	sfree(newmessage);
}

static void
xmlrpc_command_success_string(struct sourceinfo *si, const char *result, const char *message)
{
	struct connection *cptr;
	struct httpddata *hd;

	cptr = si->connection;
	hd = cptr->userdata;
	if (hd->sent_reply)
		return;
	xmlrpc_send_string(result);
	hd->sent_reply = true;
}

static struct sourceinfo_vtable xmlrpc_vtable = {
	.description = "xmlrpc",
	.cmd_fail = xmlrpc_command_fail,
	.cmd_success_nodata = xmlrpc_command_success_nodata,
	.cmd_success_string = xmlrpc_command_success_string
};

// These taken from the old modules/xmlrpc/account.c

/* atheme.login
 *
 * XML Inputs:
 *       account name, password, source ip (optional)
 *
 * XML Outputs:
 *       fault 1 - insufficient parameters
 *       fault 3 - account is not registered
 *       fault 5 - invalid username and password
 *       fault 6 - account is frozen
 *       default - success (authcookie)
 *
 * Side Effects:
 *       an authcookie ticket is created for the struct myuser.
 *       the user's lastlogin is updated
 */
static int
xmlrpcmethod_login(void *conn, int parc, char *parv[])
{
	struct myuser *mu;
	struct authcookie *ac;
	const char *sourceip;

	if (parc < 2)
	{
		xmlrpc_generic_error(fault_needmoreparams, "Insufficient parameters.");
		return 0;
	}

	sourceip = parc >= 3 && *parv[2] != '\0' ? parv[2] : NULL;

	if (!(mu = myuser_find(parv[0])))
	{
		xmlrpc_generic_error(fault_nosuch_source, "The account is not registered.");
		return 0;
	}

	if (metadata_find(mu, "private:freeze:freezer") != NULL)
	{
		logcommand_external(nicksvs.me, "xmlrpc", conn, sourceip, NULL, CMDLOG_LOGIN, "failed LOGIN to \2%s\2 (frozen)", entity(mu)->name);
		xmlrpc_generic_error(fault_noprivs, "The account has been frozen.");
		return 0;
	}

	if (!verify_password(mu, parv[1]))
	{
		struct sourceinfo *si;

		logcommand_external(nicksvs.me, "xmlrpc", conn, sourceip, NULL, CMDLOG_LOGIN, "failed LOGIN to \2%s\2 (bad password)", entity(mu)->name);
		xmlrpc_generic_error(fault_authfail, "The password is not valid for this account.");

		si = sourceinfo_create();
		si->service = NULL;
		si->sourcedesc = parv[2] != NULL && *parv[2] ? parv[2] : NULL;
		si->connection = conn;
		si->v = &xmlrpc_vtable;
		si->force_language = language_find("en");

		bad_password(si, mu);

		atheme_object_unref(si);

		return 0;
	}

	mu->lastlogin = CURRTIME;

	ac = authcookie_create(mu);

	logcommand_external(nicksvs.me, "xmlrpc", conn, sourceip, mu, CMDLOG_LOGIN, "LOGIN");

	xmlrpc_send_string(ac->ticket);

	return 0;
}

/* atheme.logout
 *
 * XML inputs:
 *       authcookie, and account name.
 *
 * XML outputs:
 *       fault 1 - insufficient parameters
 *       fault 3 - unknown user
 *       fault 15 - validation failed
 *       default - success message
 *
 * Side Effects:
 *       an authcookie ticket is destroyed.
 */
static int
xmlrpcmethod_logout(void *conn, int parc, char *parv[])
{
	struct authcookie *ac;
	struct myuser *mu;

	if (parc < 2)
	{
		xmlrpc_generic_error(fault_needmoreparams, "Insufficient parameters.");
		return 0;
	}

	if ((mu = myuser_find(parv[1])) == NULL)
	{
		xmlrpc_generic_error(fault_nosuch_source, "Unknown user.");
		return 0;
	}

	if (authcookie_validate(parv[0], mu) == false)
	{
		xmlrpc_generic_error(fault_badauthcookie, "Invalid authcookie for this account.");
		return 0;
	}

	logcommand_external(nicksvs.me, "xmlrpc", conn, NULL, mu, CMDLOG_LOGIN, "LOGOUT");

	ac = authcookie_find(parv[0], mu);
	authcookie_destroy(ac);

	xmlrpc_send_string("You are now logged out.");

	return 0;
}

/* atheme.command
 *
 * XML inputs:
 *       authcookie, account name, source ip, service name, command name,
 *       parameters.
 *
 * XML outputs:
 *       depends on command
 *
 * Side Effects:
 *       command is executed
 */
static int
xmlrpcmethod_command(void *conn, int parc, char *parv[])
{
	struct myuser *mu;
	struct service *svs;
	struct command *cmd;
	struct sourceinfo *si;
	int newparc;
	char *newparv[20];
	struct httpddata *hd = ((struct connection *)conn)->userdata;
	int i;

	for (i = 0; i < parc; i++)
	{
		if ((i >= 2 && *parv[i] == '\0') || strchr(parv[i], '\r') || strchr(parv[i], '\n'))
		{
			xmlrpc_generic_error(fault_badparams, "Invalid parameters.");
			return 0;
		}
	}

	if (parc < 5)
	{
		xmlrpc_generic_error(fault_needmoreparams, "Insufficient parameters.");
		return 0;
	}

	if (*parv[0] && *parv[1])
	{
		if ((mu = myuser_find(parv[1])) == NULL)
		{
			xmlrpc_generic_error(fault_nosuch_source, "Unknown user.");
			return 0;
		}

		if (authcookie_validate(parv[0], mu) == false)
		{
			xmlrpc_generic_error(fault_badauthcookie, "Invalid authcookie for this account.");
			return 0;
		}
	}
	else
		mu = NULL;

	// try literal service name first, then user-configured nickname.
	svs = service_find(parv[3]);
	if ((svs == NULL && (svs = service_find_nick(parv[3])) == NULL) || svs->commands == NULL)
	{
		slog(LG_DEBUG, "xmlrpcmethod_command(): invalid service %s", parv[3]);
		xmlrpc_generic_error(fault_nosuch_source, "Invalid service name.");
		return 0;
	}
	cmd = command_find(svs->commands, parv[4]);
	if (cmd == NULL)
	{
		xmlrpc_generic_error(fault_nosuch_source, "Invalid command name.");
		return 0;
	}

	memset(newparv, '\0', sizeof newparv);
	newparc = parc - 5;
	if (newparc > 20)
		newparc = 20;
	if (newparc > 0)
		memcpy(newparv, parv + 5, newparc * sizeof(parv[0]));

	si = sourceinfo_create();
	si->smu = mu;
	si->service = svs;
	si->sourcedesc = parv[2][0] != '\0' ? parv[2] : NULL;
	si->connection = conn;
	si->v = &xmlrpc_vtable;
	si->force_language = language_find("en");
	command_exec(svs, si, cmd, newparc, newparv);

	// XXX: needs to be fixed up for restartable commands...
	if (!hd->sent_reply)
	{
		if (hd->replybuf != NULL)
			xmlrpc_send_string(hd->replybuf);
		else
			xmlrpc_generic_error(fault_unimplemented, "Command did not return a result.");
	}

	atheme_object_unref(si);

	return 0;
}

/* atheme.privset
 *
 * XML inputs:
 *       authcookie, account name, source ip
 *
 * XML outputs:
 *       depends on command
 *
 * Side Effects:
 *       command is executed
 */
static int
xmlrpcmethod_privset(void *conn, int parc, char *parv[])
{
	struct myuser *mu;
	int i;

	for (i = 0; i < parc; i++)
	{
		if (strchr(parv[i], '\r') || strchr(parv[i], '\n'))
		{
			xmlrpc_generic_error(fault_badparams, "Invalid parameters.");
			return 0;
		}
	}

	if (parc < 2)
	{
		xmlrpc_generic_error(fault_needmoreparams, "Insufficient parameters.");
		return 0;
	}

	if (*parv[1] != '\0' && strlen(parv[0]) > 1)
	{
		if ((mu = myuser_find(parv[1])) == NULL)
		{
			xmlrpc_generic_error(fault_nosuch_source, "Unknown user.");
			return 0;
		}

		if (authcookie_validate(parv[0], mu) == false)
		{
			xmlrpc_generic_error(fault_badauthcookie, "Invalid authcookie for this account.");
			return 0;
		}
	}
	else
		mu = NULL;

	if (mu == NULL || !is_soper(mu))
	{
		// no privileges
		xmlrpc_send_string("");
		return 0;
	}
	xmlrpc_send_string(mu->soper->operclass->privs);

	return 0;
}

/* atheme.ison
 *
 * XML inputs:
 *       nickname
 *
 * XML outputs:
 *       boolean: if nickname is online
 *       string: if nickname is authenticated, what entity they are authed to,
 *       else '*'
 */
static int
xmlrpcmethod_ison(void *conn, int parc, char *parv[])
{
	struct user *u;
	int i;
	char buf[XMLRPC_BUFSIZE], buf2[XMLRPC_BUFSIZE];

	for (i = 0; i < parc; i++)
	{
		if (strchr(parv[i], '\r') || strchr(parv[i], '\n'))
		{
			xmlrpc_generic_error(fault_badparams, "Invalid parameters.");
			return 0;
		}
	}

	if (parc < 1)
	{
		xmlrpc_generic_error(fault_needmoreparams, "Insufficient parameters.");
		return 0;
	}

	u = user_find(parv[0]);
	if (u == NULL)
	{
		xmlrpc_boolean(buf, false);
		xmlrpc_string(buf2, "*");
		xmlrpc_send(2, buf, buf2);
		return 0;
	}

	xmlrpc_boolean(buf, true);
	xmlrpc_string(buf2, u->myuser != NULL ? entity(u->myuser)->name : "*");
	xmlrpc_send(2, buf, buf2);

	return 0;
}

/* atheme.metadata
 *
 * XML inputs:
 *       authcookie
 *       account name
 *       entity name, UID or channel name
 *       metadata key
 *       metadata value (optional)
 *
 * XML outputs:
 *       string: metadata value
 */
static int
xmlrpcmethod_metadata(void *conn, int parc, char *parv[])
{
	struct metadata *md;
	struct myuser *mu;
	int i;
	bool showprivate = false;
	char buf[XMLRPC_BUFSIZE];

	for (i = 0; i < parc; i++)
	{
		if (strchr(parv[i], '\r') || strchr(parv[i], '\n'))
		{
			xmlrpc_generic_error(fault_badparams, "Invalid parameters.");
			return 0;
		}
	}

	if (parc < 4)
	{
		xmlrpc_generic_error(fault_needmoreparams, "Insufficient parameters.");
		return 0;
	}

	if ((mu = myuser_find(parv[1])) == NULL)
	{
		xmlrpc_generic_error(fault_nosuch_source, "Unknown user.");
		return 0;
	}

	if (authcookie_validate(parv[0], mu) == false)
	{
		xmlrpc_generic_error(fault_badauthcookie, "Invalid authcookie for this account.");
		return 0;
	}

	if (*parv[2] == '#')
	{
		struct mychan *mc;

		mc = mychan_find(parv[2]);
		if (mc == NULL)
		{
			xmlrpc_generic_error(fault_nosuch_source, "No channel registration was found for the provided channel name.");
			return 0;
		}

		md = metadata_find(mc, parv[3]);
	}
	else
	{
		struct myentity *mt;

		mt = myentity_find(parv[2]);
		if (mt == NULL)
			mt = myentity_find_uid(parv[2]);

		if (mt == NULL)
		{
			xmlrpc_generic_error(fault_nosuch_source, "No account was found for this accountname or UID.");
			return 0;
		}

		md = metadata_find(mt, parv[3]);
	}

	if (md == NULL)
	{
		xmlrpc_generic_error(fault_nosuch_source, "No metadata found matching this account/channel and key.");
		return 0;
	}

	if (*parv[2] == '#' && has_priv_myuser(mu, PRIV_CHAN_AUSPEX))
		showprivate = true;
	else if (*parv[2] == '!' && has_priv_myuser(mu, PRIV_GROUP_AUSPEX))
		showprivate = true;
	else if (*parv[2] != '#' && *parv[2] != '!' && has_priv_myuser(mu, PRIV_USER_AUSPEX))
		showprivate = true;

	if (strncmp(md->name, "private:", 8) != 0 || showprivate)
		xmlrpc_string(buf, md->value);
	else
		xmlrpc_string(buf, "");
	xmlrpc_send(1, buf);

	return 0;
}

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, httpd_path_handlers, "misc/httpd", "httpd_path_handlers")

	hook_add_config_ready(xmlrpc_config_ready);

	xmlrpc_config.path = sstrdup("/xmlrpc");

	add_subblock_top_conf("XMLRPC", &conf_xmlrpc_table);
	add_dupstr_conf_item("PATH", &conf_xmlrpc_table, 0, &xmlrpc_config.path, NULL);

	xmlrpc_set_buffer(dump_buffer);
	xmlrpc_set_options(XMLRPC_HTTP_HEADER, XMLRPC_OFF);
	xmlrpc_register_method("atheme.login", xmlrpcmethod_login);
	xmlrpc_register_method("atheme.logout", xmlrpcmethod_logout);
	xmlrpc_register_method("atheme.command", xmlrpcmethod_command);
	xmlrpc_register_method("atheme.privset", xmlrpcmethod_privset);
	xmlrpc_register_method("atheme.ison", xmlrpcmethod_ison);
	xmlrpc_register_method("atheme.metadata", xmlrpcmethod_metadata);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	mowgli_node_t *n;

	xmlrpc_unregister_method("atheme.login");
	xmlrpc_unregister_method("atheme.logout");
	xmlrpc_unregister_method("atheme.command");
	xmlrpc_unregister_method("atheme.privset");
	xmlrpc_unregister_method("atheme.ison");
	xmlrpc_unregister_method("atheme.metadata");

	if ((n = mowgli_node_find(&handle_xmlrpc, httpd_path_handlers)) != NULL)
	{
		mowgli_node_delete(n, httpd_path_handlers);
		mowgli_node_free(n);
	}

	del_conf_item("PATH", &conf_xmlrpc_table);
	del_top_conf("XMLRPC");

	sfree(xmlrpc_config.path);

	hook_del_config_ready(xmlrpc_config_ready);
}

SIMPLE_DECLARE_MODULE_V1("transport/xmlrpc", MODULE_UNLOAD_CAPABILITY_OK)
