/*
 * Copyright (c) 2005 William Pitcock et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This is the header which includes all of the internationalization stuff.
 *
 * $Id: i18n.h 7869 2007-03-06 01:23:46Z jilles $
 */

#ifndef __ATHEME_INTL_H__
#define __ATHEME_INTL_H__

#ifdef ENABLE_NLS
# include <locale.h>
# include <libintl.h>
# define P_(x,y,z)	ngettext(x, y, z)
#else
# define _(x)     	(x)
# define N_(x)    	(x)
# define P_(x,y,z)	((z) != 1 ? (x) : (y))
#endif

#endif
