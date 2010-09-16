/*
 * Copyright (c) 2005-2006 Jilles Tjoelker et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * New xmlrpc implementation
 *
 */

#include "atheme.h"
#include "httpd.h"
#include "xmlrpclib.h"
#include "datastream.h"
#include "authcookie.h"

DECLARE_MODULE_V1
(
	"xmlrpc/main", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void handle_request(connection_t *cptr, void *requestbuf);

path_handler_t handle_xmlrpc = { NULL, handle_request };

struct
{
	char *path;
} xmlrpc_config;

connection_t *current_cptr; /* XXX: Hack: src/xmlrpc.c requires us to do this */

list_t *httpd_path_handlers;

static void xmlrpc_command_fail(sourceinfo_t *si, faultcode_t code, const char *message);
static void xmlrpc_command_success_nodata(sourceinfo_t *si, const char *message);
static void xmlrpc_command_success_string(sourceinfo_t *si, const char *result, const char *message);

static int xmlrpcmethod_login(void *conn, int parc, char *parv[]);
static int xmlrpcmethod_logout(void *conn, int parc, char *parv[]);
static int xmlrpcmethod_command(void *conn, int parc, char *parv[]);
static int xmlrpcmethod_privset(void *conn, int parc, char *parv[]);

/* Configuration */
list_t conf_xmlrpc_table;

struct sourceinfo_vtable xmlrpc_vtable = {
	.description = "xmlrpc",
	.cmd_fail = xmlrpc_command_fail,
	.cmd_success_nodata = xmlrpc_command_success_nodata,
	.cmd_success_string = xmlrpc_command_success_string
};

static char *dump_buffer(char *buf, int length)
{
	struct httpddata *hd;
	char buf1[300];

	hd = current_cptr->userdata;
	snprintf(buf1, sizeof buf1, "HTTP/1.1 200 OK\r\n"
			"%s"
			"Server: Atheme/%s\r\n"
			"Content-Type: text/xml\r\n"
			"Content-Length: %d\r\n\r\n",
			hd->connection_close ? "Connection: close\r\n" : "",
			PACKAGE_VERSION, length);
	sendq_add(current_cptr, buf1, strlen(buf1));
	sendq_add(current_cptr, buf, length);
	if (hd->connection_close)
		sendq_add_eof(current_cptr);
	return buf;
}

static void handle_request(connection_t *cptr, void *requestbuf)
{
	current_cptr = cptr;
	xmlrpc_process(requestbuf, cptr);
	current_cptr = NULL;

	return; 
}

static void xmlrpc_config_ready(void *vptr)
{
	/* Note: handle_xmlrpc.path may point to freed memory between
	 * reading the config and here.
	 */
	handle_xmlrpc.path = xmlrpc_config.path;

	if (handle_xmlrpc.handler != NULL)
	{
		if (node_find(&handle_xmlrpc, httpd_path_handlers))
			return;

		node_add(&handle_xmlrpc, node_create(), httpd_path_handlers);
	}
	else
		slog(LG_ERROR, "xmlrpc_config_ready(): xmlrpc {} block missing or invalid");
}

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(httpd_path_handlers, "misc/httpd", "httpd_path_handlers");

	hook_add_event("config_ready");
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
}

void _moddeinit(void)
{
	node_t *n;

	xmlrpc_unregister_method("atheme.login");
	xmlrpc_unregister_method("atheme.logout");
	xmlrpc_unregister_method("atheme.command");
	xmlrpc_unregister_method("atheme.privset");

	if ((n = node_find(&handle_xmlrpc, httpd_path_handlers)) != NULL)
	{
		node_del(n, httpd_path_handlers);
		node_free(n);
	}

	del_conf_item("PATH", &conf_xmlrpc_table);
	del_top_conf("XMLRPC");

	free(xmlrpc_config.path);

	hook_del_config_ready(xmlrpc_config_ready);
}

static void xmlrpc_command_fail(sourceinfo_t *si, faultcode_t code, const char *message)
{
	connection_t *cptr;
	struct httpddata *hd;
	const char *newmessage;

	cptr = si->connection;
	hd = cptr->userdata;
	if (hd->sent_reply)
		return;
	newmessage = xmlrpc_normalizeBuffer(message);
	xmlrpc_generic_error(code, newmessage);
	hd->sent_reply = true;
}

static void xmlrpc_command_success_nodata(sourceinfo_t *si, const char *message)
{
	connection_t *cptr;
	struct httpddata *hd;
	const char *newmessage;
	char *p;

	newmessage = xmlrpc_normalizeBuffer(message);

	cptr = si->connection;
	hd = cptr->userdata;
	if (hd->sent_reply)
		return;
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
}

static void xmlrpc_command_success_string(sourceinfo_t *si, const char *result, const char *message)
{
	connection_t *cptr;
	struct httpddata *hd;

	cptr = si->connection;
	hd = cptr->userdata;
	if (hd->sent_reply)
		return;
	xmlrpc_send_string(result);
	hd->sent_reply = true;
}

/* These taken from the old modules/xmlrpc/account.c */
/*
 * atheme.login
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
 *       an authcookie ticket is created for the myuser_t.
 *       the user's lastlogin is updated
 */
static int xmlrpcmethod_login(void *conn, int parc, char *parv[])
{
	myuser_t *mu;
	authcookie_t *ac;
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
		logcommand_external(nicksvs.me, "xmlrpc", conn, sourceip, NULL, CMDLOG_LOGIN, "failed LOGIN to \2%s\2 (bad password)", entity(mu)->name);
		xmlrpc_generic_error(fault_authfail, "The password is not valid for this account.");
		return 0;
	}

	mu->lastlogin = CURRTIME;

	ac = authcookie_create(mu);

	logcommand_external(nicksvs.me, "xmlrpc", conn, sourceip, mu, CMDLOG_LOGIN, "LOGIN");

	xmlrpc_send_string(ac->ticket);

	return 0;
}

/*
 * atheme.logout
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
static int xmlrpcmethod_logout(void *conn, int parc, char *parv[])
{
	authcookie_t *ac;
	myuser_t *mu;

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

/*
 * atheme.command
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
static int xmlrpcmethod_command(void *conn, int parc, char *parv[])
{
	myuser_t *mu;
	service_t *svs;
	command_t *cmd;
	sourceinfo_t si;
	int newparc;
	char *newparv[20];
	struct httpddata *hd = ((connection_t *)conn)->userdata;
	int i;

	for (i = 0; i < parc; i++)
	{
		if (strchr(parv[i], '\r') || strchr(parv[i], '\n'))
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

	svs = service_find_nick(parv[3]);
	if (svs == NULL || svs->cmdtree == NULL)
	{
		slog(LG_DEBUG, "xmlrpcmethod_command(): invalid service %s", parv[3]);
		xmlrpc_generic_error(fault_nosuch_source, "Invalid service name.");
		return 0;
	}
	cmd = command_find(svs->cmdtree, parv[4]);
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
	memset(&si, '\0', sizeof si);
	si.smu = mu;
	si.service = svs;
	si.sourcedesc = parv[2][0] != '\0' ? parv[2] : NULL;
	si.connection = conn;
	si.v = &xmlrpc_vtable;
	si.force_language = language_find("en");
	command_exec(svs, &si, cmd, newparc, newparv);
	if (!hd->sent_reply)
	{
		if (hd->replybuf != NULL)
			xmlrpc_send_string(hd->replybuf);
		else
			xmlrpc_generic_error(fault_unimplemented, "Command did not return a result.");
	}

	return 0;
}

/*
 * atheme.privset
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
static int xmlrpcmethod_privset(void *conn, int parc, char *parv[])
{
	myuser_t *mu;
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
		/* no privileges */
		xmlrpc_send_string("");
		return 0;
	}
	xmlrpc_send_string(mu->soper->operclass->privs);

	return 0;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs ts=8 sw=8 noexpandtab
 */
