AC_DEFUN([ATHEME_LIBTEST_QRENCODE], [

    LIBQRENCODE="No"
    LIBQRENCODE_PATH=""

    AC_ARG_WITH([qrencode],
        [AS_HELP_STRING([--without-qrencode], [Do not attempt to detect libqrencode (for generating QR codes)])],
        [], [with_qrencode="auto"])

    case "x${with_qrencode}" in
        xno | xyes | xauto)
            ;;
        x/*)
            LIBQRENCODE_PATH="${with_qrencode}"
            with_qrencode="yes"
            ;;
        *)
            AC_MSG_ERROR([invalid option for --with-qrencode])
            ;;
    esac

    CFLAGS_SAVED="${CFLAGS}"
    LIBS_SAVED="${LIBS}"

    AS_IF([test "${with_qrencode}" != "no"], [
        AS_IF([test -n "${LIBQRENCODE_PATH}"], [
            dnl Allow for user to provide custom installation directory
            AS_IF([test -d "${LIBQRENCODE_PATH}/include" -a -d "${LIBQRENCODE_PATH}/lib"], [
                LIBQRENCODE_CFLAGS="-I${LIBQRENCODE_PATH}/include"
                LIBQRENCODE_LIBS="-L${LIBQRENCODE_PATH}/lib"
            ], [
                AC_MSG_ERROR([${LIBQRENCODE_PATH} is not a suitable directory for libqrencode])
            ])
        ], [test -n "${PKG_CONFIG}"], [
            dnl Allow for the user to "override" pkg-config without it being installed
            PKG_CHECK_MODULES([LIBQRENCODE], [libqrencode], [], [])
        ])
        AS_IF([test -n "${LIBQRENCODE_CFLAGS+set}" -a -n "${LIBQRENCODE_LIBS+set}"], [
            dnl Only proceed with library tests if custom paths were given or pkg-config succeeded
            LIBQRENCODE="Yes"
        ], [
            LIBQRENCODE="No"
            AS_IF([test "${with_qrencode}" != "auto"], [
                AC_MSG_FAILURE([--with-qrencode was given but libqrencode could not be found])
            ])
        ])
    ])

    AS_IF([test "${LIBQRENCODE}" = "Yes"], [
        CFLAGS="${LIBQRENCODE_CFLAGS} ${CFLAGS}"
        LIBS="${LIBQRENCODE_LIBS} ${LIBS}"

        AC_MSG_CHECKING([if libqrencode appears to be usable])
        AC_LINK_IFELSE([
            AC_LANG_PROGRAM([[
                #ifdef HAVE_STDDEF_H
                #  include <stddef.h>
                #endif
                #include <qrencode.h>
            ]], [[
                (void) QRcode_encodeData(0, NULL, 0, (QRecLevel) 0);
                (void) QRcode_free(NULL);
            ]])
        ], [
            AC_MSG_RESULT([yes])
            LIBQRENCODE="Yes"
            AC_DEFINE([HAVE_LIBQRENCODE], [1], [Define to 1 if libqrencode appears to be usable])
            ATHEME_COND_QRCODE_ENABLE
        ], [
            AC_MSG_RESULT([no])
            LIBQRENCODE="No"
            AS_IF([test "${with_qrencode}" != "auto"], [
                AC_MSG_FAILURE([--with-qrencode was given but libqrencode does not appear to be usable])
            ])
        ])
    ])

    CFLAGS="${CFLAGS_SAVED}"
    LIBS="${LIBS_SAVED}"

    AS_IF([test "${LIBQRENCODE}" = "No"], [
        LIBQRENCODE_CFLAGS=""
        LIBQRENCODE_LIBS=""
    ])

    AC_SUBST([LIBQRENCODE_CFLAGS])
    AC_SUBST([LIBQRENCODE_LIBS])
])
