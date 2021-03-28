/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2021 Aaron M. D. Jones <me@aaronmdjones.net>
 *
 * Graphite (https://graphiteapp.org/) data exporter.
 */

#include <atheme.h>

#define GRAPHITE_INTERVAL_MIN           1U
#define GRAPHITE_INTERVAL_DEF           0U
#define GRAPHITE_INTERVAL_MAX           300U

#define GRAPHITE_RECONNECTION_INTERVAL  5U

// Used to periodically collect metrics (at the configured interval) and queue them for sending
static mowgli_eventloop_timer_t *graphite_collection_timer = NULL;
static unsigned int graphite_collection_interval = 0;

// Parameters for the active connection (if any)
static struct connection *graphite_connection = NULL;
static unsigned int graphite_connection_port = 0;
static char *graphite_connection_vhost = NULL;
static char *graphite_connection_host = NULL;
static bool graphite_connected = false;

// Parameters for new connections (from config file)
static mowgli_eventloop_timer_t *graphite_reconnection_timer = NULL;
static mowgli_list_t graphite_conf_table;
static unsigned int graphite_port = 0;
static char *graphite_vhost = NULL;
static char *graphite_host = NULL;

// Miscellaneous parameters
static unsigned int graphite_interval = 0;
static char *graphite_prefix = NULL;

static void graphite_connect(void);
static void graphite_disconnect(bool);
static void graphite_reconnect(bool);

static inline void
graphite_clear_connection(void)
{
	(void) sfree(graphite_connection_vhost);
	(void) sfree(graphite_connection_host);

	graphite_connection_vhost = NULL;
	graphite_connection_host = NULL;
	graphite_connection_port = 0;
	graphite_connection = NULL;
	graphite_connected = false;
}

static inline void
graphite_write_metric(const char *const restrict metric, const unsigned int value)
{
	char buf[BUFSIZE];

	const unsigned long long int ts = CURRTIME;
	const int len = snprintf(buf, sizeof buf, "%s.%s %u %llu\n", graphite_prefix, metric, value, ts);

	(void) sendq_add(graphite_connection, buf, (size_t) len);
}

static void
graphite_write_metrics(void)
{
	const unsigned int cnt_net_channels = mowgli_patricia_size(chanlist);
	const unsigned int cnt_net_servers  = mowgli_patricia_size(servlist);
	const unsigned int cnt_net_users    = mowgli_patricia_size(userlist);
	const unsigned int cnt_srv_channels = mowgli_patricia_size(mclist);

	unsigned int cnt_srv_accounts = 0;
	unsigned int cnt_srv_groups   = 0;

	struct myentity *mt;
	struct myentity_iteration_state state;

	MYENTITY_FOREACH_T(mt, &state, ENT_ANY)
	{
		switch (mt->type)
		{
			case ENT_ANY:
			case ENT_EXTTARGET:
				// To avoid diagnostics
				break;
			case ENT_USER:
				cnt_srv_accounts++;
				break;
			case ENT_GROUP:
				cnt_srv_groups++;
				break;
		}
	}

	(void) graphite_write_metric("network.channels",  cnt_net_channels);
	(void) graphite_write_metric("network.servers",   cnt_net_servers);
	(void) graphite_write_metric("network.users",     cnt_net_users);
	(void) graphite_write_metric("services.accounts", cnt_srv_accounts);
	(void) graphite_write_metric("services.channels", cnt_srv_channels);
	(void) graphite_write_metric("services.groups",   cnt_srv_groups);
}

static void
graphite_collection_timer_cb(void *const restrict ATHEME_VATTR_UNUSED vptr)
{
	if (graphite_connected)
		(void) graphite_write_metrics();
	else
		(void) slog(LG_DEBUG, "%s: no active connection", MOWGLI_FUNC_NAME);
}

static void
graphite_timer_setup(void)
{
	if (graphite_collection_timer)
		(void) mowgli_timer_destroy(base_eventloop, graphite_collection_timer);

	graphite_collection_interval = graphite_interval;
	graphite_collection_timer = mowgli_timer_add(base_eventloop, "graphite_collection_timer",
	                                             &graphite_collection_timer_cb, NULL,
	                                             graphite_collection_interval);
}

static void
graphite_read_cb(struct connection *const restrict cptr)
{
	return_if_fail(cptr == graphite_connection);

	unsigned char buf[BUFSIZE];
	const ssize_t ret = read(cptr->fd, buf, sizeof buf);

	if (ret == -1 && ! mowgli_eventloop_ignore_errno(errno))
	{
		(void) slog(LG_ERROR, "%s: error on Graphite socket: %s", MOWGLI_FUNC_NAME, strerror(errno));
		(void) graphite_reconnect(false);
		return;
	}
	if (ret == 0)
	{
		(void) slog(LG_INFO, "%s: lost connection to Graphite", MOWGLI_FUNC_NAME);
		(void) graphite_reconnect(false);
		return;
	}
}

static void
graphite_write_cb(struct connection *const restrict cptr)
{
	return_if_fail(cptr == graphite_connection);

	(void) slog(LG_INFO, "%s: connected (%s)", MOWGLI_FUNC_NAME, cptr->name);

	graphite_connected = true;

	(void) connection_setselect_write(cptr, NULL);
	(void) graphite_write_metrics();
}

static void
graphite_connect(void)
{
	(void) graphite_clear_connection();

	(void) slog(LG_INFO, "%s: connecting", MOWGLI_FUNC_NAME);

	graphite_connection = connection_open_tcp(graphite_host, graphite_vhost, graphite_port, &graphite_read_cb,
	                                          &graphite_write_cb);

	if (graphite_connection)
	{
		graphite_connection_vhost = sstrdup(graphite_vhost);
		graphite_connection_host = sstrdup(graphite_host);
		graphite_connection_port = graphite_port;
	}
	else
		(void) graphite_reconnect(false);
}

static void
graphite_disconnect(const bool expected)
{
	if (graphite_connected)
	{
		(void) slog(LG_INFO, "%s: disconnecting", MOWGLI_FUNC_NAME);

		if (expected)
		{
			(void) slog(LG_DEBUG, "%s: flushing sendq", MOWGLI_FUNC_NAME);
			(void) sendq_flush(graphite_connection);
			(void) slog(LG_DEBUG, "%s: done", MOWGLI_FUNC_NAME);
		}
	}

	if (graphite_connection)
		(void) connection_close(graphite_connection);

	(void) graphite_clear_connection();
}

static void
graphite_reconnect_cb(void *const restrict ATHEME_VATTR_UNUSED vptr)
{
	graphite_reconnection_timer = NULL;

	(void) graphite_connect();
}

static void
graphite_reconnect(const bool expected)
{
	(void) graphite_disconnect(expected);

	if (graphite_reconnection_timer)
	{
		(void) mowgli_timer_destroy(base_eventloop, graphite_reconnection_timer);

		graphite_reconnection_timer = NULL;
	}

	if (expected)
		(void) graphite_connect();
	else
		graphite_reconnection_timer = mowgli_timer_add_once(base_eventloop, "graphite_reconnect",
		                                                    &graphite_reconnect_cb, NULL,
		                                                    GRAPHITE_RECONNECTION_INTERVAL);
}

static void
graphite_config_ready(void *const restrict ATHEME_VATTR_UNUSED vptr)
{
	if (! (graphite_prefix && *graphite_prefix))
	{
		(void) slog(LG_ERROR, "%s: no prefix specified", MOWGLI_FUNC_NAME);
		return;
	}
	if (! (graphite_host && *graphite_host))
	{
		(void) slog(LG_ERROR, "%s: no host specified", MOWGLI_FUNC_NAME);
		return;
	}
	if (! graphite_port)
	{
		(void) slog(LG_ERROR, "%s: no port specified, or not within 1-65535 (inclusive)", MOWGLI_FUNC_NAME);
		return;
	}
	if (! graphite_interval)
	{
		(void) slog(LG_ERROR, "%s: no interval specified", MOWGLI_FUNC_NAME);
		return;
	}

	if (graphite_collection_interval != graphite_interval)
	{
		if (graphite_collection_interval)
			(void) slog(LG_INFO, "%s: collection interval has changed", MOWGLI_FUNC_NAME);

		(void) graphite_timer_setup();
	}

	if (graphite_connection)
	{
		bool reconnect = false;

		if (strcasecmp(graphite_connection_vhost, graphite_vhost) != 0)
			reconnect = true;

		if (strcasecmp(graphite_connection_host, graphite_host) != 0)
			reconnect = true;

		if (graphite_connection_port != graphite_port)
			reconnect = true;

		if (reconnect)
			(void) graphite_reconnect(true);
	}
	else
		(void) graphite_connect();
}

static void
mod_init(struct module *const restrict ATHEME_VATTR_UNUSED m)
{
	(void) hook_add_config_ready(&graphite_config_ready);

	(void) add_subblock_top_conf("GRAPHITE", &graphite_conf_table);
	(void) add_dupstr_conf_item("PREFIX", &graphite_conf_table, 0, &graphite_prefix, NULL);
	(void) add_dupstr_conf_item("VHOST", &graphite_conf_table, 0, &graphite_vhost, NULL);
	(void) add_dupstr_conf_item("HOST", &graphite_conf_table, 0, &graphite_host, NULL);
	(void) add_uint_conf_item("PORT", &graphite_conf_table, 0, &graphite_port, 1U, 65535U, 0U);
	(void) add_uint_conf_item("INTERVAL", &graphite_conf_table, 0, &graphite_interval,
	                          GRAPHITE_INTERVAL_MIN, GRAPHITE_INTERVAL_MAX, GRAPHITE_INTERVAL_DEF);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	if (graphite_reconnection_timer)
		(void) mowgli_timer_destroy(base_eventloop, graphite_reconnection_timer);

	if (graphite_collection_timer)
		(void) mowgli_timer_destroy(base_eventloop, graphite_collection_timer);

	(void) graphite_disconnect(true);

	(void) hook_del_config_ready(&graphite_config_ready);

	(void) del_conf_item("PREFIX", &graphite_conf_table);
	(void) del_conf_item("VHOST", &graphite_conf_table);
	(void) del_conf_item("HOST", &graphite_conf_table);
	(void) del_conf_item("PORT", &graphite_conf_table);
	(void) del_conf_item("INTERVAL", &graphite_conf_table);
	(void) del_top_conf("GRAPHITE");
}

SIMPLE_DECLARE_MODULE_V1("statserv/graphite", MODULE_UNLOAD_CAPABILITY_OK)
