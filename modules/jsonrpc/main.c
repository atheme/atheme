/*
 * Copyright (c) 2005-2006 Jilles Tjoelker et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * JSON-RPC path handler for misc/httpd.
 *
 * $Id: main.c 8405 2007-06-03 21:40:48Z pippijn $
 */

#include "atheme.h"
#include "httpd.h"
#include "datastream.h"
#include "authcookie.h"
#include "jsonrpc.h"
#include "json/json.h"
#include "json/rpc/jsonrpc.h"

DECLARE_MODULE_V1
(
	"jsonrpc/main", FALSE, _modinit, _moddeinit,
	"$Id: main.c 8405 2007-06-03 21:40:48Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void handle_request(connection_t *cptr, void *requestbuf);

path_handler_t handle_jsonrpc = { NULL, handle_request };

list_t *httpd_path_handlers;

static void jsonrpc_command_fail(sourceinfo_t *si, faultcode_t code, const char *message);
static void jsonrpc_command_success_nodata(sourceinfo_t *si, const char *message);
static void jsonrpc_command_success_string(sourceinfo_t *si, const char *result, const char *message);

static void jsonrpcmethod_login(connection_t *cptr, json_object_t *request, struct json_object *response);
static void jsonrpcmethod_logout(connection_t *cptr, json_object_t *request, struct json_object *response);
static void jsonrpcmethod_command(connection_t *cptr, json_object_t *request, struct json_object *response);

/* Configuration */
list_t conf_jsonrpc_table;
static int conf_jsonrpc_path(config_entry_t *ce)
{
	if (!ce->ce_vardata)
		return -1;

	handle_jsonrpc.path = sstrdup(ce->ce_vardata);

	return 0;
}

static int conf_jsonrpc(config_entry_t *ce)
{
	subblock_handler(ce, &conf_jsonrpc_table);
	return 0;
}

struct sourceinfo_vtable jsonrpc_vtable = {
	"jsonrpc",
	jsonrpc_command_fail,
	jsonrpc_command_success_nodata,
	jsonrpc_command_success_string
};

static char *dump_buffer(connection_t *cptr, char *buf, int length)
{
	struct httpddata *hd;
	char buf1[300];

	hd = cptr->userdata;
	snprintf(buf1, sizeof buf1, "HTTP/1.1 200 OK\r\n"
			"%s"
			"Server: Atheme/%s\r\n"
			"Content-Type: application/json\r\n"
			"Content-Length: %d\r\n\r\n",
			hd->connection_close ? "Connection: close\r\n" : "",
			version, length);
	sendq_add(cptr, buf1, strlen(buf1));
	sendq_add(cptr, buf, length);
	if (hd->connection_close)
		sendq_add_eof(cptr);
	return buf;
}

static void handle_request(connection_t *cptr, void *requestbuf)
{
	char *response;
	response = jsonrpc_process(cptr, requestbuf);
	dump_buffer(cptr, response, strlen(response));
	
	return; 
}

static void jsonrpc_config_ready(void *vptr)
{
	node_t *n;

	if (handle_jsonrpc.path == NULL)
		handle_jsonrpc.path = "/jsonrpc";
	
	if ((n = node_find(&handle_jsonrpc, httpd_path_handlers)))
	{
		slog(LG_INFO, "jsonrpc/main.c: handler already in the list");
		return;
	}
	
	n = node_create();
	node_add(&handle_jsonrpc, node_create(), httpd_path_handlers);
}

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(httpd_path_handlers, "misc/httpd", "httpd_path_handlers");

	hook_add_event("config_ready");
	hook_add_hook("config_ready", jsonrpc_config_ready);

	add_top_conf("JSONRPC", conf_jsonrpc);
	add_conf_item("PATH", &conf_jsonrpc_table, conf_jsonrpc_path);

	jsonrpc_add_method("atheme.login", jsonrpcmethod_login);
	jsonrpc_add_method("atheme.logout", jsonrpcmethod_logout);
	jsonrpc_add_method("atheme.command", jsonrpcmethod_command);
}

void _moddeinit(void)
{
	node_t *n;

#if 0
	jsonrpc_unregister_method("atheme.login");
	jsonrpc_unregister_method("atheme.logout");
	jsonrpc_unregister_method("atheme.command");
#endif

	if (!(n = node_find(&handle_jsonrpc, httpd_path_handlers)))
	{
		slog(LG_INFO, "jsonrpc/main.c: handler was not registered.");
		return;
	}

	node_del(n, httpd_path_handlers);
	node_free(n);

	del_conf_item("PATH", &conf_jsonrpc_table);
	del_top_conf("JSONRPC");
}

static void jsonrpc_command_fail(sourceinfo_t *si, faultcode_t code, const char *message)
{
	char *response;
	json_object_t *error_ob;
	connection_t *cptr;
	struct httpddata *hd;
	char newmessage[256];
	int i;
	const char *p;
	
	error_ob = json_object_new_object();
	cptr = si->connection;
	hd = cptr->userdata;

	if (hd->sent_reply)
		return;
	i = 0, p = message;
	while (i < 255 && *p != '\0')
		if (*p > '\0' && *p < ' ')
			p++;
		else
			newmessage[i++] = *p, p++;
	newmessage[i] = '\0';

	jsonrpc_generic_error(error_ob, code, newmessage);
	response = json_object_to_json_string(error_ob);
	dump_buffer(cptr, response, strlen(response));

	free(error_ob);

	hd->sent_reply = TRUE;
}

static void jsonrpc_command_success_nodata(sourceinfo_t *si, const char *message)
{
	connection_t *cptr;
	struct httpddata *hd;
	char *p;
	const char *q;

	cptr = si->connection;
	hd = cptr->userdata;
	if (hd->sent_reply)
		return;
	if (hd->replybuf != NULL)
	{
		hd->replybuf = srealloc(hd->replybuf, strlen(hd->replybuf) + strlen(message) + 2);
		p = hd->replybuf + strlen(hd->replybuf);
		*p++ = '\n';
	}
	else
	{
		hd->replybuf = smalloc(strlen(message) + 1);
		p = hd->replybuf;
	}
	q = message;
	while (*q != '\0')
		if (*q > '\0' && *q < ' ')
			q++;
		else
			*p++ = *q++;
	*p = '\0';
}

static void jsonrpc_command_success_string(sourceinfo_t *si, const char *result, const char *message)
{
	char *response;
	json_object_t *json_msg_ob;
	connection_t *cptr;
	struct httpddata *hd;

	json_msg_ob = json_object_new_object();
	cptr = si->connection;
	hd = cptr->userdata;

	if (hd->sent_reply)
		return;

	json_object_object_add(json_msg_ob, "message", json_object_new_string(message));
	response = json_object_to_json_string(json_msg_ob);
	dump_buffer(cptr, response, strlen(response));

	hd->sent_reply = TRUE;
}

/*
 * atheme.login
 *
 * JSON Inputs:
 *       account name, password, source ip (optional)
 *
 * JSON Outputs:
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
static void jsonrpcmethod_login(connection_t *cptr, json_object_t *request, json_object_t *response)
{
	json_object_t *params, *entry;
	char *username, *password;
	const char *sourceip;
	myuser_t *mu;
	authcookie_t *ac;
	
	params = json_object_object_get(request, "params");

	if (json_object_array_length(params) < 2)
	{
		jsonrpc_generic_error(response, fault_needmoreparams, _("Insufficient parameters."));
		return;
	}

	username = json_object_get_string(json_object_array_get_idx(params, 0));
	password = json_object_get_string(json_object_array_get_idx(params, 1));
	sourceip = json_object_get_string(json_object_array_get_idx(params, 2));

	if (!(mu = myuser_find(username)))
	{
		jsonrpc_generic_error(response, fault_nosuch_source, _("The account is not registered."));
		return;
	}

	if (metadata_find(mu, METADATA_USER, "private:freeze:freezer") != NULL)
	{
		logcommand_external(nicksvs.me, "jsonrpc", cptr, sourceip, NULL, CMDLOG_LOGIN, "failed LOGIN to %s (frozen)", mu->name);
		jsonrpc_generic_error(response, fault_noprivs, _("The account has been frozen."));
		return;
	}

	if (!verify_password(mu, password))
	{
		logcommand_external(nicksvs.me, "jsonrpc", cptr, sourceip, NULL, CMDLOG_LOGIN, "failed LOGIN to %s (bad password)", mu->name);
		jsonrpc_generic_error(response, fault_authfail, _("The password is not valid for this account."));
		return;
	}

	mu->lastlogin = CURRTIME;

	ac = authcookie_create(mu);

	logcommand_external(nicksvs.me, "jsonrpc", cptr, sourceip, mu, CMDLOG_LOGIN, "LOGIN");

	entry = json_object_new_object();
	json_object_object_add(entry, "authcookie", json_object_new_string(ac->ticket));

	json_object_object_add(response, "result", entry);
}

/*
 * atheme.logout
 *
 * JSON inputs:
 *       authcookie, and account name.
 *
 * JSON outputs:
 *       fault 1 - insufficient parameters
 *       fault 3 - unknown user
 *       fault 5 - validation failed
 *       default - success message
 *
 * Side Effects:
 *       an authcookie ticket is destroyed.
 */
static void jsonrpcmethod_logout(connection_t *cptr, struct json_object *request, struct json_object *response)
{
	json_object_t *params, *entry;
	char *cookie, *username;
	authcookie_t *ac;
	myuser_t *mu;

	params = json_object_object_get(request, "params");

	if (json_object_array_length(params) < 2)
	{
		jsonrpc_generic_error(response, fault_needmoreparams, _("Insufficient parameters."));
		return;
	}

	cookie   = json_object_get_string(json_object_array_get_idx(params, 0));
	username = json_object_get_string(json_object_array_get_idx(params, 1));

	if ((mu = myuser_find(username)) == NULL)
	{
		jsonrpc_generic_error(response, fault_nosuch_source, _("Unknown user."));
		return;
	}

	if (authcookie_validate(cookie, mu) == FALSE)
	{
		jsonrpc_generic_error(response, fault_authfail, _("Invalid authcookie for this account."));
		return;
	}

	logcommand_external(nicksvs.me, "jsonrpc", cptr, NULL, mu, CMDLOG_LOGIN, "LOGOUT");

	ac = authcookie_find(cookie, mu);
	authcookie_destroy(ac);

	entry = json_object_new_object();
	json_object_object_add(entry, "message", json_object_new_string(_("You are now logged out")));

	json_object_object_add(response, "result", entry);
}

/*
 * atheme.command
 *
 * JSON inputs:
 *       authcookie, account name, source ip, service name, command name,
 *       parameters.
 *
 * JSON outputs:
 *       depends on command
 *
 * Side Effects:
 *       command is executed
 */
static void jsonrpcmethod_command(connection_t *cptr, struct json_object *request, struct json_object *response)
{
	json_object_t *params, *entry;
	myuser_t *mu;
	service_t *svs;
	command_t *cmd;
	sourceinfo_t si;
	int newparc, parc;
	char *newparv[MAX_CMD_PARC], *parv[MAX_CMD_PARC];
	struct httpddata *hd = cptr->userdata;
	int i;

	memset(parv, '\0', sizeof parv);
	params = (json_object_t *)malloc(sizeof(json_object_t *));

	params = json_object_object_get(request, "params");
	parc = json_object_array_length(params);

	if (parc < 5)
	{
		jsonrpc_generic_error(response, fault_needmoreparams, _("Insufficient parameters."));
		return;
	}

	for (i = 0; i < parc; i++)
	{
		parv[i] = json_object_get_string(json_object_array_get_idx(params, i));
		if (parv[i] == NULL)
			parv[i] = "(null)";
	}

	for (i = 0; i < parc; i++)
	{
		if (strchr(parv[i], '\r') || strchr(parv[i], '\n'))
		{
			jsonrpc_generic_error(response, fault_badparams, _("Invalid parameters."));
			return;
		}
	}

	if (*parv[1] != '\0' && strlen(parv[0]) > 1)
	{
		if ((mu = myuser_find(parv[1])) == NULL)
		{
			jsonrpc_generic_error(response, fault_nosuch_source, _("Unknown user."));
			return;
		}

		if (authcookie_validate(parv[0], mu) == FALSE)
		{
			jsonrpc_generic_error(response, fault_authfail, _("Invalid authcookie for this account."));
			return;
		}
	}
	else
		mu = NULL;

	svs = find_service(parv[3]);
	if (svs == NULL || svs->cmdtree == NULL)
	{
		slog(LG_DEBUG, "jsonrpcmethod_command(): invalid service %s", parv[3]);
		jsonrpc_generic_error(response, fault_nosuch_source, _("Invalid service name."));
		return;
	}
	cmd = command_find(svs->cmdtree, parv[4]);
	if (cmd == NULL)
	{
		jsonrpc_generic_error(response, fault_nosuch_target, _("Invalid command name."));
		return;
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
	si.connection = cptr;
	si.v = &jsonrpc_vtable;
	command_exec(svs, &si, cmd, newparc, newparv);
	if (!hd->sent_reply)
	{
		if (hd->replybuf != NULL)
		{
			entry = json_object_new_object();
			json_object_object_add(entry, "message", json_object_new_string(hd->replybuf));
			
			json_object_object_add(response, "result", entry);
		}
		else
			jsonrpc_generic_error(response, fault_unimplemented, _("Command did not return a result."));
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs ts=8 sw=8 noexpandtab
 */
