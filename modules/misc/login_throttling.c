/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2022 Atheme Development Group (https://atheme.github.io/)
 */

#include <atheme.h>

#define LT_WEIGHT_MIN   0U
#define LT_WEIGHT_MAX   300U
#define LT_WEIGHT_DEF   1U

#define LT_MAX_MIN      1U
#define LT_MAX_MAX      1800U
#define LT_MAX_DEF      2U

#define MAX_ADDR_LEN    56U // strlen("2001:0db8:0000:0000:0000:0000:0000:0001%abcdefghabcdefgh")

struct lt_bucket
{
	time_t          timestamp;
	char            key[MAX_ADDR_LEN + 1 + IDLEN + 1];
};

static mowgli_list_t lt_config_table;
static mowgli_patricia_t *lt_buckets = NULL;
static mowgli_eventloop_timer_t *lt_expire_timer = NULL;

static unsigned int lt_address_account_weight = 0U;
static unsigned int lt_address_account_max = 0U;
static unsigned int lt_address_weight = 0U;
static unsigned int lt_address_max = 0U;

static inline bool
lt_deny_common(const time_t currts, const char *const restrict key,
               const unsigned int vweight, const unsigned int vmax)
{
	if (! vweight)
		return false;

	struct lt_bucket *bucket = mowgli_patricia_retrieve(lt_buckets, key);

	if (! bucket)
	{
		bucket = smalloc(sizeof *bucket);

		(void) mowgli_strlcpy(bucket->key, key, sizeof bucket->key);
		(void) mowgli_patricia_add(lt_buckets, bucket->key, bucket);

		return false;
	}

	const time_t vcap = (currts + vmax);

	if (bucket->timestamp < currts)
		bucket->timestamp = currts;

	if ((bucket->timestamp += vweight) <= vcap)
		return false;

	if (bucket->timestamp > vcap)
		bucket->timestamp = vcap;

	return true;
}

static bool
lt_deny_iplogin(const time_t currts, const char *const restrict ipaddr,
                struct myuser ATHEME_VATTR_UNUSED *const restrict mu)
{
	return lt_deny_common(currts, ipaddr, lt_address_weight, lt_address_max);
}

static bool
lt_deny_ipacctlogin(const time_t currts, const char *const restrict ipaddr,
                    struct myuser *const restrict mu)
{
	char key[BUFSIZE];

	(void) snprintf(key, sizeof key, "%s/%s", ipaddr, entity(mu)->id);

	return lt_deny_common(currts, key, lt_address_account_weight, lt_address_account_max);
}

static void
lt_user_can_login_hook(struct hook_user_login_check *const restrict hdata)
{
	static const struct {
		const char *    type;
		bool          (*func)(time_t, const char *, struct myuser *);
	} checks[] = {
		{         "IPADDR", &lt_deny_iplogin     },
		{ "IPADDR/ACCOUNT", &lt_deny_ipacctlogin },
	};

	unsigned char addrbytes[16];
	const char *ipaddr = NULL;

	return_if_fail(hdata != NULL);
	return_if_fail(hdata->si != NULL);
	return_if_fail(hdata->mu != NULL);

	if (hdata->method != HULM_PASSWORD)
		// We only throttle password-based login attempts
		return;

	if (! hdata->allowed)
		// Another hook already decided to deny the login -- nothing to do
		return;

	if (hdata->si->su)
		// This will be true for a login attempt received via NickServ
		ipaddr = hdata->si->su->ip;
	else if (hdata->si->sourcedesc)
		// This will be true for a login attempt received via SASL
		ipaddr = hdata->si->sourcedesc;

	if (! ipaddr)
		// We can't determine their IP address
		return;

	if (inet_pton(AF_INET, ipaddr, addrbytes) == 1)
	{
		// Do Nothing
	}
	else if (inet_pton(AF_INET6, ipaddr, addrbytes) == 1)
	{
		// Do Nothing
	}
	else
		// Invalid IP address
		return;

	const time_t currts = time(NULL);

	for (size_t i = 0; i < ARRAY_SIZE(checks); i++)
	{
		if (! ((*(checks[i].func))(currts, ipaddr, hdata->mu)))
			continue;

		(void) slog(LG_VERBOSE, "LOGIN:THROTTLE:%s: \2%s\2 (\2%s\2)",
		                        checks[i].type, entity(hdata->mu)->name, ipaddr);

		hdata->allowed = false;
		return;
	}
}

static void
lt_operserv_info_hook(struct sourceinfo *const restrict si)
{
	const unsigned int vsize = mowgli_patricia_size(lt_buckets);

	(void) command_success_nodata(si, _("Number of login throttling entries: %u"), vsize);
}

static void
lt_expire_timer_cb(void ATHEME_VATTR_UNUSED *const restrict unused)
{
	mowgli_patricia_iteration_state_t state;
	struct lt_bucket *bucket;

	const time_t expirets = (time(NULL) - SECONDS_PER_HOUR);

	MOWGLI_PATRICIA_FOREACH(bucket, &state, lt_buckets)
	{
		if (bucket->timestamp > expirets)
			continue;

		(void) mowgli_patricia_delete(lt_buckets, bucket->key);
		(void) sfree(bucket);
	}
}

static void
lt_patricia_destroy_cb(const char ATHEME_VATTR_UNUSED *const restrict key,
                       void *const restrict bucket,
                       void ATHEME_VATTR_UNUSED *const restrict unused)
{
	(void) sfree(bucket);
}

static void
mod_init(struct module *const restrict m)
{
	if (! (lt_buckets = mowgli_patricia_create(&strcasecanon)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_patricia_create() failed", m->name);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	if (! (lt_expire_timer = mowgli_timer_add(base_eventloop, "login_throttle_expire_timer",
	                                          &lt_expire_timer_cb, NULL, SECONDS_PER_HOUR)))
	{
		(void) slog(LG_ERROR, "%s: mowgli_timer_add() failed", m->name);

		(void) mowgli_patricia_destroy(lt_buckets, NULL, NULL);

		m->mflags |= MODFLAG_FAIL;
		return;
	}

	(void) hook_add_operserv_info(&lt_operserv_info_hook);
	(void) hook_add_user_can_login(&lt_user_can_login_hook);

	(void) add_subblock_top_conf("throttle", &lt_config_table);
	(void) add_uint_conf_item("address_account_weight", &lt_config_table, 0, &lt_address_account_weight,
	                          LT_WEIGHT_MIN, LT_WEIGHT_MAX, LT_WEIGHT_DEF);
	(void) add_uint_conf_item("address_account_max", &lt_config_table, 0, &lt_address_account_max,
	                          LT_MAX_MIN, LT_MAX_MAX, LT_MAX_DEF);
	(void) add_uint_conf_item("address_weight", &lt_config_table, 0, &lt_address_weight,
	                          LT_WEIGHT_MIN, LT_WEIGHT_MAX, LT_WEIGHT_DEF);
	(void) add_uint_conf_item("address_max", &lt_config_table, 0, &lt_address_max,
	                          LT_MAX_MIN, LT_MAX_MAX, LT_MAX_DEF);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) del_conf_item("address_account_weight", &lt_config_table);
	(void) del_conf_item("address_account_max", &lt_config_table);
	(void) del_conf_item("address_weight", &lt_config_table);
	(void) del_conf_item("address_max", &lt_config_table);
	(void) del_top_conf("throttle");

	(void) hook_del_operserv_info(&lt_operserv_info_hook);
	(void) hook_del_user_can_login(&lt_user_can_login_hook);
	(void) mowgli_timer_destroy(base_eventloop, lt_expire_timer);
	(void) mowgli_patricia_destroy(lt_buckets, &lt_patricia_destroy_cb, NULL);
}

SIMPLE_DECLARE_MODULE_V1("misc/login_throttling", MODULE_UNLOAD_CAPABILITY_OK)
