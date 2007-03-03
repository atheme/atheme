/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Symbols which are relocated at runtime relating to database activities.
 *
 * $Id: dbhandler.c 7779 2007-03-03 13:55:42Z pippijn $
 */

#include "atheme.h"

void (*db_save) (void *arg) = NULL;
void (*db_load) (void) = NULL;

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
