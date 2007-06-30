dnl
dnl Check for dlopen symbol and set DYNAMIC_LD_LIBS.
dnl
dnl AM_DL()
dnl

AC_DEFUN([AM_DL], [
  AC_CHECK_LIB(c, dlopen,
   [DYNAMIC_LD_LIBS=""
    have_dl=yes])

  if test x$have_dl != "xyes"; then
    AC_CHECK_LIB(dl, dlopen,
     [DYNAMIC_LD_LIBS="-ldl"
      have_dl=yes])
  fi

  if test x$have_dl != "xyes"; then
    AC_MSG_CHECKING(for dlopen under win32)
    AC_LANG_SAVE()
    AC_LANG_C()

    ac_save_CPPFLAGS="$CPPFLAGS"
    ac_save_LIBS="$LIBS"
    CPPFLAGS="-I${srcdir}/win32/include $CPPFLAGS"
    LIBS="$LIBS -lkernel32"
    AC_COMPILE_IFELSE([
#include <stddef.h>
#include <dlfcn.h>

int main() {
  dlopen(NULL, 0);
  return 0;
}
], 
      [DYNAMIC_LD_LIBS=-lkernel32
       have_dl=yes
       AC_MSG_RESULT(yes)],
       AC_MSG_RESULT(no)
    )

    CPPFLAGS=$ac_save_CPPFLAGS
    LIBS=$ac_save_LIBS

    AC_LANG_RESTORE()
  fi

  if test x$have_dl != "xyes"; then
    AC_MSG_ERROR(dynamic linker needed)
  fi

  AC_SUBST(DYNAMIC_LD_LIBS)

])
