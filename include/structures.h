/*
 * Copyright (C) 2018 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Structures used throughout the project (forward declarations).
 */

#ifndef STRUCTURES_H
#define STRUCTURES_H

// XXX remove me, or move me to include/hook.h
typedef struct user_ user_t;
typedef struct server_ server_t;
typedef struct soper_ soper_t;
typedef struct myuser_ myuser_t;
typedef struct mynick_ mynick_t;
typedef struct mychan_ mychan_t;

// Defined in include/channels.h
struct chanban;
struct channel;
struct chanuser;

// Defined in include/connection.h
struct connection;

// Defined in include/crypto.h
struct crypt_impl;

// Defined in include/culture.h
struct language;

// Defined in include/database_backend.h
struct database_handle;
struct database_module;
struct database_vtable;

// Defined in include/entity-validation.h
struct entity_chanacs_validation_vtable;

// Defined in include/servtree.h
struct service;

// Defined in include/sourceinfo.h
struct sourceinfo;
struct sourceinfo_vtable;

#endif
