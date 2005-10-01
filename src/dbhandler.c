/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Symbols which are relocated at runtime relating to database activities.
 *
 * $Id: dbhandler.c 2497 2005-10-01 04:35:25Z nenolod $
 */

#include "atheme.h"

void (*db_save) (void *arg) = NULL;
void (*db_load) (void) = NULL;
