/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2006 Atheme Project (http://atheme.org/)
 *
 * Data structures for sourceinfo
 */

#ifndef ATHEME_INC_SOURCEINFO_H
#define ATHEME_INC_SOURCEINFO_H 1

#include <atheme/object.h>
#include <atheme/stdheaders.h>
#include <atheme/structures.h>

struct sourceinfo_vtable
{
	const char *    description;
	const char *  (*format)(struct sourceinfo *si, bool full);
	void          (*cmd_fail)(struct sourceinfo *si, enum cmd_faultcode code, const char *message);
	void          (*cmd_success_nodata)(struct sourceinfo *si, const char *message);
	void          (*cmd_success_string)(struct sourceinfo *si, const char *result, const char *message);
	void          (*cmd_success_table)(struct sourceinfo *si, struct atheme_table *table);
	const char *  (*get_source_name)(struct sourceinfo *si);
	const char *  (*get_source_mask)(struct sourceinfo *si);
	const char *  (*get_oper_name)(struct sourceinfo *si);
	const char *  (*get_storage_oper_name)(struct sourceinfo *si);
};

/* structure describing data about a protocol message or service command */
struct sourceinfo
{
	struct atheme_object            parent;

	/* Fields describing the source of the message
	 *
	 * For protocol modules, the following applies to su and s:
	 *
	 * 1) At most one of these two can be non-NULL
	 * 2) Before server registration, both are NULL
	 * 3) For services commands, s is always NULL; and su
	 *    is non-NULL if the command was received via IRC.
	 *
	 * su is the source user, s is the source server.
	 */
	struct user *                   su;
	struct server *                 s;

	struct connection *             connection;     // physical connection cmd received from
	const char *                    sourcedesc;     // additional information (e.g. IP address)
	struct myuser *                 smu;            // login associated with source

	/* The service the original command was sent to, which may
	 * differ from the service the current command is in.
	 */
	struct service *                service;

	struct channel *                c;              // channel this command applies to (fantasy?)
	struct sourceinfo_vtable *      v;              // function pointers, could be NULL
	void *                          callerdata;     // opaque data pointer for caller
	unsigned int                    output_limit;   // if not 0, limit lines of output
	unsigned int                    output_count;   // lines of output upto now
	struct language *               force_language; // locale to force replies to be in, could be NULL
	struct command *                command;        // The command being executed
};

#endif /* !ATHEME_INC_SOURCEINFO_H */
