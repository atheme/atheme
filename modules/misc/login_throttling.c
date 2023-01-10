/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2022 Atheme Development Group (https://atheme.github.io/)
 */

#include <atheme.h>

#define LT_BURST_MIN            0U
#define LT_BURST_MAX            200U
#define LT_REPLENISH_MIN        0.005
#define LT_REPLENISH_MAX        200.0

#define LT_BURST_IP_DEF         5U
#define LT_REPLENISH_IP_DEF     1.0

#define LT_BURST_IPACCT_DEF     2U
#define LT_REPLENISH_IPACCT_DEF 0.5

#define MAX_ADDR_LEN            56U // strlen("2001:0db8:0000:0000:0000:0000:0000:0001%abcdefghabcdefgh")

struct lt_bucket
{
	double  timestamp;
	char    key[MAX_ADDR_LEN + 1 + IDLEN + 1];
};

static mowgli_list_t lt_config_table;
static mowgli_patricia_t *lt_buckets = NULL;
static mowgli_eventloop_timer_t *lt_expire_timer = NULL;

static unsigned int lt_address_account_burst = 0U;
static double lt_address_account_replenish = 0.0;
static unsigned int lt_address_burst = 0U;
static double lt_address_replenish = 0.0;

static inline bool
lt_deny_common(const double currts, const char *const restrict key,
               const unsigned int vburst, const double vreplenish)
{
	if (! vburst)
		return false;

	struct lt_bucket *bucket = mowgli_patricia_retrieve(lt_buckets, key);

	if (! bucket)
	{
		bucket = smalloc(sizeof *bucket);

		(void) mowgli_strlcpy(bucket->key, key, sizeof bucket->key);
		(void) mowgli_patricia_add(lt_buckets, bucket->key, bucket);
	}

	/* bucket->timestamp tells us when our bucket will next be totally
	 * replenished (i.e. when all throttles are gone.)
	 *
	 * if bucket->timestamp is in the past, we've fully replenished, so
	 * we want to start counting from *now*
	 */
	if (bucket->timestamp < currts)
		bucket->timestamp = currts;

	/** `bucket->timestamp + vreplenish`
	 *
	 * if we were to take one token from the bucket (i.e. attempt a login)
	 * right now, our bucket would then be considered totally replenished
	 * $vreplenish seconds from now; i.e. if vreplenish was 2, that means
	 * 1 token would replenish every 2 seconds, which means in 2 seconds
	 * from now, our bucket would be totally replenished (at the default
	 * unthrottled state)
	 *
	 ** `currts + (vreplenish * vburst)
	 *
	 * vburst represents how many tokens we can take from the bucket
	 * instantly. if vreplenish is 2, and vburst is 2, we can take a token
	 * (i.e. attempt a login) twice instantly without being throttled, but
	 * a third token would fail, and we'd have to wait $vreplenish (2)
	 * seconds for a token to be available, or we can wait
	 * `vreplenish * vburst` seconds (`2 * 2`) for the bucket to be totally
	 * replenished (i.e. at the default unthrottled state)
	 *
	 * so if `bucket->timestamp + vreplenish` is more than
	 * `currts + (vreplenish * vburst)`, our bucket does not have a token
	 * left for us to take, because if we tried to take one, it would push
	 * our bucket above the maximum "at what time will our bucket next be
	 * totally replenished"
	 */
	if ((bucket->timestamp + vreplenish) > (currts + (vreplenish * vburst)))
		return true;

	bucket->timestamp += vreplenish;
	return false;
}

static bool
lt_deny_iplogin(const double currts, const char *const restrict ipaddr,
                struct myuser ATHEME_VATTR_UNUSED *const restrict mu)
{
	return lt_deny_common(currts, ipaddr, lt_address_burst, lt_address_replenish);
}

static bool
lt_deny_ipacctlogin(const double currts, const char *const restrict ipaddr,
                    struct myuser *const restrict mu)
{
	char key[BUFSIZE];

	(void) snprintf(key, sizeof key, "%s/%s", ipaddr, entity(mu)->id);

	return lt_deny_common(currts, key, lt_address_account_burst, lt_address_account_replenish);
}

static void
lt_user_can_login_hook(struct hook_user_login_check *const restrict hdata)
{
	static const struct {
		const char *    type;
		bool          (*func)(double, const char *, struct myuser *);
	} checks[] = {
		{         "IPADDR", &lt_deny_iplogin     },
		{ "IPADDR/ACCOUNT", &lt_deny_ipacctlogin },
	};

	unsigned char addrbytes[16];
	const char *ipaddr = NULL;

	return_if_fail(hdata != NULL);
	return_if_fail(hdata->si != NULL);
	return_if_fail(hdata->mu != NULL);

	if (! hdata->allowed)
		// Another hook already decided to deny the login -- nothing to do
		return;

	if (hdata->method != HULM_PASSWORD)
		// We only throttle password-based login attempts
		return;

	if (hdata->si->su && is_ircop(hdata->si->su))
		// Don't throttle opers
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
		if (! ((*(checks[i].func))((double) currts, ipaddr, hdata->mu)))
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

	const time_t currts = time(NULL);

	MOWGLI_PATRICIA_FOREACH(bucket, &state, lt_buckets)
	{
		if (((time_t) bucket->timestamp) > currts)
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
	(void) add_uint_conf_item("address_account_burst", &lt_config_table, 0, &lt_address_account_burst,
	                          LT_BURST_MIN, LT_BURST_MAX, LT_BURST_IPACCT_DEF);
	(void) add_double_conf_item("address_account_replenish", &lt_config_table, 0, &lt_address_account_replenish,
	                          LT_REPLENISH_MIN, LT_REPLENISH_MAX, LT_REPLENISH_IPACCT_DEF);
	(void) add_uint_conf_item("address_burst", &lt_config_table, 0, &lt_address_burst,
	                          LT_BURST_MIN, LT_BURST_MAX, LT_BURST_IP_DEF);
	(void) add_double_conf_item("address_replenish", &lt_config_table, 0, &lt_address_replenish,
	                          LT_REPLENISH_MIN, LT_REPLENISH_MAX, LT_REPLENISH_IP_DEF);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	(void) del_conf_item("address_account_burst", &lt_config_table);
	(void) del_conf_item("address_account_replenish", &lt_config_table);
	(void) del_conf_item("address_burst", &lt_config_table);
	(void) del_conf_item("address_replenish", &lt_config_table);
	(void) del_top_conf("throttle");

	(void) hook_del_operserv_info(&lt_operserv_info_hook);
	(void) hook_del_user_can_login(&lt_user_can_login_hook);
	(void) mowgli_timer_destroy(base_eventloop, lt_expire_timer);
	(void) mowgli_patricia_destroy(lt_buckets, &lt_patricia_destroy_cb, NULL);
}

SIMPLE_DECLARE_MODULE_V1("misc/login_throttling", MODULE_UNLOAD_CAPABILITY_OK)
