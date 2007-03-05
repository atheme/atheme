dnl Copyright (c) 2005 William Pitcock, et al.
dnl Rights to this code are as documented in doc/LICENSE.
dnl
dnl This file contains autoconf macros used in configure.
dnl
dnl $Id: atheme-c.m4 7825 2007-03-05 23:44:42Z nenolod $

dnl Stolen from hyperion.

dnl ATHEME_C_GCC_TRY_FLAGS(<warnings>,<cachevar>)
AC_DEFUN([ATHEME_C_GCC_TRY_FLAGS],[
 AC_MSG_CHECKING([GCC flag(s) $1])
 if test "${GCC-no}" = yes
 then
  AC_CACHE_VAL($2,[
   oldcflags="${CFLAGS-}"
   CFLAGS="${CFLAGS-} ${CWARNS} $1 -Werror"
   AC_TRY_COMPILE([
#include <string.h>
#include <stdio.h>
int main(void);
],[
    strcmp("a","b"); fprintf(stdout,"test ok\n");
], [$2=yes], [$2=no])
   CFLAGS="${oldcflags}"])
  if test "x$$2" = xyes; then
   CWARNS="${CWARNS}$1 "
   AC_MSG_RESULT(ok)  
  else
   $2=''
   AC_MSG_RESULT(no)
  fi
 else
  AC_MSG_RESULT(no, not using GCC)
 fi
])
