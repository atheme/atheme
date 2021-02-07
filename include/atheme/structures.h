/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Structures used throughout the project (forward declarations).
 */

#ifndef ATHEME_INC_STRUCTURES_H
#define ATHEME_INC_STRUCTURES_H 1

// Defined in atheme/account.h
struct chanacs;
struct groupacs;
struct mychan;
struct mygroup;
struct mynick;
struct myuser;
struct svsignore;

// Defined in atheme/botserv.h
struct botserv_bot;
struct botserv_main_symbols;

// Defined in atheme/channels.h
struct chanban;
struct channel;
struct chanuser;

// Defined in atheme/connection.h
struct connection;

// Defined in atheme/crypto.h
struct crypt_impl;

// Defined in atheme/culture.h
struct language;
struct translation;

// Defined in atheme/database_backend.h
struct database_handle;
struct database_module;
struct database_vtable;

// Defined in atheme/digest*.h
struct digest_context;
struct digest_vector;

// Defined in atheme/entity.h
struct myentity;
struct myentity_iteration_state;

// Defined in atheme/entity-validation.h
struct entity_vtable;

// Defined in atheme/flags.h
struct gflags;

// Defined in atheme/global.h
struct me;

// Defined in atheme/hook.h
struct hook;

// Defined in atheme/httpd.h
struct path_handler;

// Defined in atheme/match.h
struct atheme_regex;

// Defined in atheme/module.h
struct module;
struct v4_moduleheader;

// Defined in atheme/object.h
struct atheme_object;
struct metadata;

// Defined in atheme/pcommand.h
struct proto_cmd;

// Defined in atheme/phandler.h
struct ircd;

// Defined in atheme/privs.h
struct operclass;
struct soper;

// Defined in atheme/res.h
struct nsaddr;
struct res_dns_query;
struct res_dns_reply;

// Defined in atheme/sasl.h
struct sasl_core_functions;
struct sasl_mechanism;
struct sasl_message;
struct sasl_session;
struct sasl_sourceinfo;

// Defined in atheme/servers.h
struct server;
struct tld;

// Defined in atheme/services.h
struct chansvs;
struct nicksvs;

// Defined in atheme/servtree.h
struct service;

// Defined in atheme/sharedheap.h
struct sharedheap;

// Defined in atheme/sourceinfo.h
struct sourceinfo;
struct sourceinfo_vtable;

// Defined in atheme/table.h
struct atheme_table;
struct atheme_table_cell;
struct atheme_table_row;

// Defined in atheme/taint.h
struct taint_reason;

// Defined in atheme/template.h
struct default_template;

// Defined in atheme/tools.h
struct email_canonicalizer_item;
struct logfile;

// Defined in atheme/uid.h
struct uid_provider;

// Defined in atheme/uplink.h
struct uplink;

// Defined in atheme/users.h
struct user;

#endif /* !ATHEME_INC_STRUCTURES_H */
