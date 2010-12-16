/*
 * Copyright (C) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for sourceinfo
 *
 */

#ifndef SOURCEINFO_H
#define SOURCEINFO_H

struct sourceinfo_vtable
{
	const char *description;
	void (*cmd_fail)(sourceinfo_t *si, faultcode_t code, const char *message);
	void (*cmd_success_nodata)(sourceinfo_t *si, const char *message);
	void (*cmd_success_string)(sourceinfo_t *si, const char *result, const char *message);
	const char *(*get_source_name)(sourceinfo_t *si);
	const char *(*get_source_mask)(sourceinfo_t *si);
	const char *(*get_oper_name)(sourceinfo_t *si);
	const char *(*get_storage_oper_name)(sourceinfo_t *si);
};

/* structure describing data about a protocol message or service command */
struct sourceinfo_
{
	/* fields describing the source of the message */
	/* for protocol modules, the following applies to su and s:
	 * at most one of these two can be non-NULL
	 * before server registration, both are NULL, otherwise exactly
	 * one is NULL.
	 * for services commands, s is always NULL and su is non-NULL if
	 * and only if the command was received via IRC.
	 */
	user_t *su; /* source, if it's a user */
	server_t *s; /* source, if it's a server */

	connection_t *connection; /* physical connection cmd received from */
	const char *sourcedesc; /* additional information (e.g. IP address) */
	myuser_t *smu; /* login associated with source */

	/* the service the original command was sent to, which may differ
	 * from the service the current command is in
	 */
	service_t *service;

	channel_t *c; /* channel this command applies to (fantasy?) */

	struct sourceinfo_vtable *v; /* function pointers, could be NULL */
	void *callerdata; /* opaque data pointer for caller */

	unsigned int output_limit; /* if not 0, limit lines of output */
	unsigned int output_count; /* lines of output upto now */

	language_t *force_language; /* locale to force replies to be in, could be NULL */
};

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
