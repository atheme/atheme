/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2014 Atheme Project (http://atheme.org/)
 *
 * JSONRPC implementation
 */

#include <atheme.h>
#include "jsonrpclib.h"

static mowgli_list_t *httpd_path_handlers = NULL;
static mowgli_patricia_t *json_methods = NULL;

void
jsonrpc_register_method(const char *method_name, jsonrpc_method_fn method)
{
	mowgli_patricia_add(json_methods, method_name, method);
}

void
jsonrpc_unregister_method(const char *method_name)
{
	mowgli_patricia_delete(json_methods, method_name);
}

jsonrpc_method_fn
get_json_method(const char *method_name)
{
	return mowgli_patricia_retrieve(json_methods, method_name);
}

static void
jsonrpc_command_fail(struct sourceinfo *si, enum cmd_faultcode code, const char *message)
{
	struct connection *cptr;
	struct httpddata *hd;
	char *newmessage;

	cptr = si->connection;
	hd = cptr->userdata;
	if (hd->sent_reply)
		return;
	newmessage = jsonrpc_normalizeBuffer(message);

	struct jsonrpc_sourceinfo *jsi = (struct jsonrpc_sourceinfo *)si;

	jsonrpc_failure_string(cptr, code, newmessage, jsi->id);

	sfree(newmessage);
	hd->sent_reply = true;
}

static void
jsonrpc_command_success_string(struct sourceinfo *si, const char *result, const char *message)
{
	struct connection *cptr;
	struct httpddata *hd;

	cptr = si->connection;
	hd = cptr->userdata;
	if (hd->sent_reply)
		return;

	struct jsonrpc_sourceinfo *jsi = (struct jsonrpc_sourceinfo *)si;

	jsonrpc_success_string(cptr, result, jsi->id);
	hd->sent_reply = true;
}

static void
jsonrpc_command_success_nodata(struct sourceinfo *si, const char *message)
{
	struct connection *cptr;
	struct httpddata *hd;
	char *p;

	char *newmessage = jsonrpc_normalizeBuffer(message);

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

static struct sourceinfo_vtable jsonrpc_vtable = {
	.description        = "jsonrpc",
	.cmd_fail           = jsonrpc_command_fail,
	.cmd_success_string = jsonrpc_command_success_string,
	.cmd_success_nodata = jsonrpc_command_success_nodata
};

// These taken from modules/transport/xmlrpc/main.c

/* atheme.login
 *
 * Parameters:
 *       account name, password, source ip (optional)
 *
 * Outputs:
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
static bool
jsonrpcmethod_login(void *conn, mowgli_list_t *params, char *id)
{
	struct myuser *mu;
	struct authcookie *ac;
	char *sourceip, *accountname, *password;

	size_t len = MOWGLI_LIST_LENGTH(params);

	if (len < 2)
	{
		jsonrpc_failure_string(conn, fault_needmoreparams, "Insufficient parameters.", id);
		return false;
	}

	accountname = mowgli_node_nth_data(params, 0);
	password = mowgli_node_nth_data(params, 1);
	sourceip = len >= 3 ? mowgli_node_nth_data(params, 2) : NULL;

	if (!(mu = myuser_find(accountname)))
	{
		jsonrpc_failure_string(conn, fault_nosuch_source, "The account is not registered.", id);
		return false;
	}

	if (metadata_find(mu, "private:freeze:freezer") != NULL)
	{
		logcommand_external(nicksvs.me, "jsonrpc", conn, sourceip, NULL, CMDLOG_LOGIN, "failed LOGIN to \2%s\2 (frozen)", entity(mu)->name);
		jsonrpc_failure_string(conn, fault_noprivs, "The account has been frozen.", id);
		return false;
	}

	if (!verify_password(mu, password))
	{
		struct sourceinfo *si;

		logcommand_external(nicksvs.me, "jsonrpc", conn, sourceip, NULL, CMDLOG_LOGIN, "failed LOGIN to \2%s\2 (bad password)", entity(mu)->name);
		jsonrpc_failure_string(conn, fault_authfail, "The password is incorrect.", id);

		si = sourceinfo_create();

		struct jsonrpc_sourceinfo *jsi = (struct jsonrpc_sourceinfo *)si;

		si->service = NULL;
		si->sourcedesc = sourceip;
		si->connection = conn;
		si->v = &jsonrpc_vtable;
		si->force_language = language_find("en");

		jsi->base = si;
		jsi->id = id;

		bad_password(si, mu);

		atheme_object_unref(si);

		return false;
	}

	mu->lastlogin = CURRTIME;

	ac = authcookie_create(mu);

	logcommand_external(nicksvs.me, "jsonrpc", conn, sourceip, mu, CMDLOG_LOGIN, "LOGIN");

	jsonrpc_success_string(conn, ac->ticket, id);

	return true;
}

/* atheme.logout
 *
 * JSON inputs:
 *       authcookie, and account name.
 *
 * JSON outputs:
 *       fault 1 - insufficient parameters
 *       fault 3 - unknown user
 *       fault 15 - validation failed
 *       default - success message
 *
 * JSON Effects:
 *       an authcookie ticket is destroyed.
 */
static bool
jsonrpcmethod_logout(void *conn, mowgli_list_t *params, char *id)
{
	struct authcookie *ac;
	struct myuser *mu;
	char *accountname;
	char *cookie;

	size_t len = MOWGLI_LIST_LENGTH(params);
	cookie = mowgli_node_nth_data(params, 0);
	accountname = mowgli_node_nth_data(params, 1);

	if (len < 2)
	{
		jsonrpc_failure_string(conn, fault_needmoreparams, "Insufficient parameters.", id);
		return false;
	}

	if ((mu = myuser_find(accountname)) == NULL)
	{
		jsonrpc_failure_string(conn, fault_nosuch_source, "Unknown user.", id);
		return false;
	}

	if (authcookie_validate(cookie, mu) == false)
	{
		jsonrpc_failure_string(conn, fault_badauthcookie, "Invalid authcookie for this account.", id);
		return false;
	}

	logcommand_external(nicksvs.me, "jsonrpc", conn, NULL, mu, CMDLOG_LOGIN, "LOGOUT");

	ac = authcookie_find(cookie, mu);
	authcookie_destroy(ac);

	jsonrpc_success_string(conn, "You are now logged out.", id);

	return 0;
}

/* atheme.command
 *
 * JSON inputs:
 *       authcookie, account name, source ip, service name, command name,
 *       parameters.
 *
 * JSON outputs:
 *       depends on command
 *
 * JSON Effects:
 *       command is executed
 */
static bool
jsonrpcmethod_command(void *conn, mowgli_list_t *params, char *id)
{
	struct myuser *mu;
	struct service *svs;
	struct command *cmd;
	struct sourceinfo *si;
	int newparc;
	char *newparv[20];
	struct httpddata *hd = ((struct connection *)conn)->userdata;

	struct authcookie *ac;
	char *accountname, *cookie, *service, *command, *sourceip;

	size_t len = MOWGLI_LIST_LENGTH(params);
	cookie = mowgli_node_nth_data(params, 0);
	accountname = mowgli_node_nth_data(params, 1);
	sourceip = mowgli_node_nth_data(params, 2);
	service = mowgli_node_nth_data(params, 3);
	command = mowgli_node_nth_data(params, 4);

	mowgli_node_t *n;
	char *param;

	MOWGLI_LIST_FOREACH(n, params->head)
	{
		param = n->data;

		if (*param == '\0' || strchr(param, '\r') || strchr(param, '\n'))
		{
			jsonrpc_failure_string(conn, fault_badparams, "Invalid authcookie for this account.", id);
			return 0;
		}
	}

	if (len < 5)
	{
		jsonrpc_failure_string(conn, fault_needmoreparams, "Insufficient parameters.", id);
		return 0;
	}

	if (*accountname != '\0' && strlen(cookie) > 1)
	{
		if ((mu = myuser_find(accountname)) == NULL)
		{
			jsonrpc_failure_string(conn, fault_nosuch_source, "Unknown user.", id);
			return 0;
		}

		if (authcookie_validate(cookie, mu) == false)
		{
			jsonrpc_failure_string(conn, fault_badauthcookie, "Invalid authcookie for this account.", id);
			return 0;
		}
	}
	else
		mu = NULL;

	// try literal service name first, then user-configured nickname.
	svs = service_find(service);
	if ((svs == NULL && (svs = service_find_nick(service)) == NULL) || svs->commands == NULL)
	{
		slog(LG_DEBUG, "xmlrpcmethod_command(): invalid service %s", service);
		jsonrpc_failure_string(conn, fault_nosuch_source, "Invalid service name.", id);
		return 0;
	}
	cmd = command_find(svs->commands, command);
	if (cmd == NULL)
	{
		jsonrpc_failure_string(conn, fault_nosuch_source, "Invalid command name.", id);
		return 0;
	}

	//command = mowgli_node_nth_data(params, 4);

	memset(newparv, '\0', sizeof newparv);
	newparc = len;
	if (newparc > 20)
		newparc = 20;

	for (int i = 5; i < newparc; i++) {
		newparv[i-5] = mowgli_node_nth_data(params, i);
	}

	si = sourceinfo_create();

	struct jsonrpc_sourceinfo *jsi = (struct jsonrpc_sourceinfo *)si;

	si->smu = mu;
	si->service = svs;
	si->sourcedesc = sourceip[0] != '\0' ? sourceip : NULL;
	si->connection = conn;
	si->v = &jsonrpc_vtable;
	si->force_language = language_find("en");

	jsi->base = si;
	jsi->id = id;

	command_exec(svs, si, cmd, newparc-5, newparv);

	// XXX: needs to be fixed up for restartable commands...
	if (!hd->sent_reply)
	{
		if (hd->replybuf != NULL)
			jsonrpc_success_string(conn, hd->replybuf, id);
		else
			jsonrpc_failure_string(conn, fault_unimplemented, "Command did not return a result", id);
	}

	atheme_object_unref(si);

	return 0;
}

/* atheme.privset
 *
 * JSON inputs:
 *       authcookie, account name
 *
 * JSON outputs:
 *       Privset for user, or error message
 *
*/
static bool
jsonrpcmethod_privset(void *conn, mowgli_list_t *params, char *id)
{
	struct myuser *mu;
	mowgli_node_t *n;

	char *param, *accountname, *cookie;

	size_t len = MOWGLI_LIST_LENGTH(params);
	cookie = mowgli_node_nth_data(params, 0);
	accountname = mowgli_node_nth_data(params, 1);

	MOWGLI_LIST_FOREACH(n, params->head)
	{
		param = n->data;

		if (*param == '\0' || strchr(param, '\r') || strchr(param, '\n'))
		{
			jsonrpc_failure_string(conn, fault_badparams, "Invalid authcookie for this account.", id);
			return 0;
		}
	}

	if (len < 2)
	{
		jsonrpc_failure_string(conn, fault_needmoreparams, "Insufficient parameters.", id);
		return 0;
	}

	if (*accountname != '\0' && strlen(cookie) > 1)
	{
		if ((mu = myuser_find(accountname)) == NULL)
		{
			jsonrpc_failure_string(conn, fault_nosuch_source, "Unknown user.", id);
			return 0;
		}

		if (authcookie_validate(cookie, mu) == false)
		{
			jsonrpc_failure_string(conn, fault_badauthcookie, "Invalid authcookie for this account.", id);
			return 0;
		}
	}
	else
		mu = NULL;

	if (mu == NULL || !is_soper(mu))
	{
		jsonrpc_success_string(conn, "", id);
		return 0;
	}

	jsonrpc_success_string(conn, mu->soper->operclass->privs, id);

	return 0;
}

/* atheme.ison
 *
 * JSON inputs:
 *       nickname
 *
 * JSON outputs:
 *       An object with the following properties:
 *       online: boolean: if nickname is online
 *       accountname: string: if nickname is authenticated, what entity they
 *       are authed to, else '*'
 */
static bool
jsonrpcmethod_ison(void *conn, mowgli_list_t *params, char *id)
{
	struct user *u;

	char *param, *user;
	user = mowgli_node_nth_data(params, 0);

	size_t len = MOWGLI_LIST_LENGTH(params);
	mowgli_node_t *n;

	MOWGLI_LIST_FOREACH(n, params->head)
	{
		param = n->data;

		if (*param == '\0' || strchr(param, '\r') || strchr(param, '\n'))
		{
			jsonrpc_failure_string(conn, fault_badparams, "Invalid authcookie for this account.", id);
			return 0;
		}
	}


	if (len < 1)
	{
		jsonrpc_failure_string(conn, fault_needmoreparams, "Insufficient parameters.", id);
		return 0;
	}

	u = user_find(user);
	if (u == NULL)
	{
		mowgli_json_t *resultobj = mowgli_json_create_object();
		mowgli_patricia_t *patricia = MOWGLI_JSON_OBJECT(resultobj);

		mowgli_patricia_add(patricia, "online", mowgli_json_false);
		mowgli_patricia_add(patricia, "accountname", mowgli_json_create_string("*"));

		mowgli_json_t *obj = mowgli_json_create_object();
		patricia = MOWGLI_JSON_OBJECT(obj);

		mowgli_json_t *idobj = mowgli_json_create_string(id);

		mowgli_patricia_add(patricia, "result", resultobj);
		mowgli_patricia_add(patricia, "id", idobj);
		mowgli_patricia_add(patricia, "error", mowgli_json_null);

		mowgli_string_t *str = mowgli_string_create();

		mowgli_json_serialize_to_string(obj, str, 0);

		jsonrpc_send_data(conn, str->str);

		return 0;
	}

	mowgli_json_t *resultobj = mowgli_json_create_object();
	mowgli_patricia_t *patricia = MOWGLI_JSON_OBJECT(resultobj);

	mowgli_patricia_add(patricia, "online", mowgli_json_true);
	mowgli_patricia_add(patricia, "accountname",  mowgli_json_create_string(u->myuser != NULL ? entity(u->myuser)->name : "*"));

	mowgli_json_t *obj = mowgli_json_create_object();
	patricia = MOWGLI_JSON_OBJECT(obj);

	mowgli_json_t *idobj = mowgli_json_create_string(id);

	mowgli_patricia_add(patricia, "result", resultobj);
	mowgli_patricia_add(patricia, "id", idobj);
	mowgli_patricia_add(patricia, "error", mowgli_json_null);

	mowgli_string_t *str = mowgli_string_create();

	mowgli_json_serialize_to_string(obj, str, 0);

	jsonrpc_send_data(conn, str->str);

	return 0;
}

/* atheme.metadata
 *
 * JSON inputs:
 *       entity name, UID or channel name
 *       metadata key
 *
 * JSON outputs:
 *       metadata value
 */
static bool
jsonrpcmethod_metadata(void *conn, mowgli_list_t *params, char *id)
{
	struct metadata *md;

	char *param, *name, *metadata;

	name = mowgli_node_nth_data(params, 0);
	metadata = mowgli_node_nth_data(params, 1);

	size_t len = MOWGLI_LIST_LENGTH(params);
	mowgli_node_t *n;

	MOWGLI_LIST_FOREACH(n, params->head)
	{
		param = n->data;

		if (*param == '\0' || strchr(param, '\r') || strchr(param, '\n'))
		{
			jsonrpc_failure_string(conn, fault_badparams, "Invalid authcookie for this account.", id);
			return 0;
		}
	}

	if (len < 2)
	{
		jsonrpc_failure_string(conn, fault_needmoreparams, "Insufficient parameters.", id);
		return 0;
	}

	if (*name == '#')
	{
		struct mychan *mc;

		mc = mychan_find(name);
		if (mc == NULL)
		{
			jsonrpc_failure_string(conn, fault_nosuch_source, "No channel registration was found for the provided channel name.", id);
			return 0;
		}

		md = metadata_find(mc, metadata);
	}
	else
	{
		struct myentity *mt;

		mt = myentity_find(name);
		if (mt == NULL)
			mt = myentity_find_uid(name);

		if (mt == NULL)
		{
			jsonrpc_failure_string(conn, fault_nosuch_source, "No account was found for this accountname or UID.", id);
			return 0;
		}

		md = metadata_find(mt, metadata);
	}

	if (md == NULL)
	{
		jsonrpc_failure_string(conn, fault_nosuch_source, "No metadata found matching this account/channel and key.", id);
		return 0;
	}

	jsonrpc_success_string(conn, md->value, id);

	return 0;
}

void
jsonrpc_send_data(void *conn, char *str)
{
	struct httpddata *hd = ((struct connection *) conn)->userdata;

	char buf[300];

	size_t len = strlen(str);

	snprintf(buf, sizeof buf,
	         "HTTP/1.1 200 OK\r\n"
	         "Server: %s/%s\r\n"
	         "Content-Type: application/json\r\n"
	         "Content-Length: %zu\r\n"
	         "%s"
	         "\r\n",
	         PACKAGE_TARNAME, PACKAGE_VERSION,
	         len,
	         hd->connection_close ? "Connection: close\r\n" : "");

	sendq_add((struct connection *)conn, buf, strlen(buf));
	sendq_add((struct connection *)conn, str, len);

	if (hd->connection_close) {
		sendq_add_eof((struct connection *) conn);
	}
}

static void
handle_request(struct connection *cptr, void *requestbuf)
{
	jsonrpc_process(requestbuf, cptr);

	return;
}

static struct path_handler handle_jsonrpc = { NULL, handle_request };

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, httpd_path_handlers, "misc/httpd", "httpd_path_handlers")

	handle_jsonrpc.path = "/jsonrpc";
	mowgli_node_add(&handle_jsonrpc, mowgli_node_create(), httpd_path_handlers);

	json_methods = mowgli_patricia_create(strcasecanon);

	jsonrpc_register_method("atheme.login", jsonrpcmethod_login);
	jsonrpc_register_method("atheme.logout", jsonrpcmethod_logout);
	jsonrpc_register_method("atheme.command", jsonrpcmethod_command);

	jsonrpc_register_method("atheme.privset", jsonrpcmethod_privset);
	jsonrpc_register_method("atheme.ison", jsonrpcmethod_ison);
	jsonrpc_register_method("atheme.metadata", jsonrpcmethod_metadata);

}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	mowgli_node_t *n;

	jsonrpc_unregister_method("atheme.login");
	jsonrpc_unregister_method("atheme.logout");
	jsonrpc_unregister_method("atheme.command");

	jsonrpc_unregister_method("atheme.privset");
	jsonrpc_unregister_method("atheme.ison");
	jsonrpc_unregister_method("atheme.metadata");

	if ((n = mowgli_node_find(&handle_jsonrpc, httpd_path_handlers)) != NULL)
	{
		mowgli_node_delete(n, httpd_path_handlers);
		mowgli_node_free(n);
	}
}

SIMPLE_DECLARE_MODULE_V1("transport/jsonrpc", MODULE_UNLOAD_CAPABILITY_OK)
