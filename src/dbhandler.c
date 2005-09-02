/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Symbols which are relocated at runtime relating to database activities.
 *
 * $Id: dbhandler.c 1156 2005-07-27 01:26:17Z alambert $
 */

#include "atheme.h"

void (*db_save)(void *arg) = NULL;
void (*db_load)(void) = NULL;

