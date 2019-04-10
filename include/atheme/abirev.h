/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2008 Jilles Tjoelker
 *
 * Module ABI revision.
 */

#ifndef ATHEME_INC_ABIREV_H
#define ATHEME_INC_ABIREV_H 1

/*
 * Increment this in case of changes to the module ABI (i.e. changes
 * that require modules to be recompiled).
 * When incrementing, if the first two digits do not agree with the
 * current major.minor version (e.g. 3.3-devel), change the first two
 * digits and set the rest to 0 (e.g. 330000). Otherwise, increment
 * the lower digits.
 */
#define CURRENT_ABI_REVISION 730000U

#endif /* !ATHEME_INC_ABIREV_H */
