/*
 * Copyright (c) 2008 Jilles Tjoelker
 * The rights to this code are as documented in doc/LICENSE.
 *
 * Module ABI revision.
 */

#ifndef ABIREV_H
#define ABIREV_H

/*
 * Increment this in case of changes to the module ABI (i.e. changes
 * that require modules to be recompiled).
 * When incrementing, if the first two digits do not agree with the
 * current major.minor version (e.g. 3.3-devel), change the first two
 * digits and set the rest to 0 (e.g. 330000). Otherwise, increment
 * the lower digits.
 */
#define CURRENT_ABI_REVISION 600000

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
