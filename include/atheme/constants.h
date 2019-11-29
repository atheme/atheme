/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2006 Atheme Project (http://atheme.org/)
 * Copyright (C) 2019 Atheme Development Group (https://atheme.github.io/)
 *
 * Constants used by multiple header files.
 */

#ifndef ATHEME_INC_CONSTANTS_H
#define ATHEME_INC_CONSTANTS_H 1

#include <atheme/i18n.h>
#include <atheme/stdheaders.h>

#ifdef ATHEME_ENABLE_LARGE_NET
#  define HEAP_NODE             1024U
#  define HEAP_CHANNEL          1024U
#  define HEAP_CHANUSER         1024U
#  define HEAP_USER             1024U
#  define HEAP_SERVER           16U
#  define HEAP_CHANACS          1024U
#  define HASH_USER             65535U
#  define HASH_CHANNEL          32768U
#  define HASH_SERVER           128U
#else /* ATHEME_ENABLE_LARGE_NET */
#  define HEAP_NODE             1024U
#  define HEAP_CHANNEL          64U
#  define HEAP_CHANUSER         128U
#  define HEAP_USER             128U
#  define HEAP_SERVER           8U
#  define HEAP_CHANACS          128U
#  define HASH_USER             1024U
#  define HASH_CHANNEL          512U
#  define HASH_SERVER           32U
#endif /* !ATHEME_ENABLE_LARGE_NET */

#define HASH_COMMAND            256U
#define HASH_SMALL              32U
#define HASH_ITRANS             128U
#define HASH_TRANS              2048U

#define BUFSIZE                 1024U
#define MAXMODES                4U
#define MAX_IRC_OUTPUT_LINES    2000U

/* lengths of various pieces of information (without NULL terminators) */
#define HOSTLEN                 63U
#define NICKLEN                 50U
#define PASSLEN                 288U
#define IDLEN                   9U
#define UIDLEN                  16U
#define CHANNELLEN              200U
#define GROUPLEN                31U
#define USERLEN                 11U
#define HOSTIPLEN               53U
#define GECOSLEN                50U
#define KEYLEN                  23U
#define EMAILLEN                254U
#define MEMOLEN                 300U

#define MAXMSIGNORES            40U

#define CACHEFILE_HEAP_SIZE     32U
#define CACHELINE_HEAP_SIZE     64U

#define FLOOD_MSGS_FACTOR       256U
#define FLOOD_HEAVY             (3U * FLOOD_MSGS_FACTOR)
#define FLOOD_MODERATE          FLOOD_MSGS_FACTOR
#define FLOOD_LIGHT             0U

#define SECONDS_PER_MINUTE      60U
#define SECONDS_PER_HOUR        3600U
#define SECONDS_PER_DAY         86400U
#define SECONDS_PER_WEEK        604800U
#define MINUTES_PER_HOUR        60U
#define MINUTES_PER_DAY         1440U
#define MINUTES_PER_WEEK        10080U
#define HOURS_PER_DAY           24U
#define HOURS_PER_WEEK          168U
#define DAYS_PER_WEEK           7U

#ifndef TIME_FORMAT
#  define TIME_FORMAT           "%b %d %H:%M:%S %Y %z"
#endif

#define STR_CHANNEL_IS_CLOSED   _("\2%s\2 is closed.")
#define STR_CHANNEL_IS_EMPTY    _("\2%s\2 is currently empty.")
#define STR_EMAIL_NOT_VERIFIED  _("You must verify your e-mail address before you may perform this operation.")
#define STR_HELP_DESCRIPTION    N_("Displays contextual help information.")
#define STR_INSUFFICIENT_PARAMS _("Insufficient parameters for \2%s\2.")
#define STR_INVALID_PARAMS      _("Invalid parameters for \2%s\2.")
#define STR_IRC_COMMAND_ONLY    _("\2%s\2 can only be executed via IRC.")
#define STR_IS_NOT_REGISTERED   _("\2%s\2 is not registered.")
#define STR_NO_PRIVILEGE        _("You do not have the \2%s\2 privilege.")
#define STR_NOT_AUTHORIZED      _("You are not authorized to perform this operation.")
#define STR_NOT_LOGGED_IN       _("You are not logged in.")

#endif /* !ATHEME_INC_CONSTANTS_H */
