# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2005-2009 Atheme Project (http://atheme.org/)
# Copyright (C) 2018-2019 Atheme Development Group (https://atheme.github.io/)
#
# -*- Atheme IRC Services -*-
# Atheme Build System Component

AC_DEFUN([ATHEME_FEATURETEST_LDFLAGS], [

    AC_ARG_ENABLE([profile],
        [AS_HELP_STRING([--enable-profile], [Enable profiling extensions])],
        [], [enable_profile="no"])

    AC_ARG_ENABLE([relro],
        [AS_HELP_STRING([--disable-relro], [Disable -Wl,-z,relro (marks the relocation table read-only)])],
        [], [enable_relro="yes"])

    AC_ARG_ENABLE([nonlazy-bind],
        [AS_HELP_STRING([--disable-nonlazy-bind], [Disable -Wl,-z,now (resolves all symbols at program startup time)])],
        [], [enable_nonlazy_bind="yes"])

    AC_ARG_ENABLE([linker-defs],
        [AS_HELP_STRING([--disable-linker-defs], [Disable -Wl,-z,defs (detects and rejects underlinking)])],
        [], [enable_linker_defs="yes"])

    AC_ARG_ENABLE([noexecstack],
        [AS_HELP_STRING([--disable-noexecstack], [Disable -Wl,-z,noexecstack (Marks the stack non-executable)])],
        [], [enable_noexecstack="yes"])

    AC_ARG_ENABLE([as-needed],
        [AS_HELP_STRING([--disable-as-needed], [Disable -Wl,--as-needed (strips unnecessary libraries at link time)])],
        [], [enable_as_needed="yes"])

    AC_ARG_ENABLE([rpath],
        [AS_HELP_STRING([--disable-rpath], [Disable -Wl,-rpath= (builds installation path into binaries)])],
        [], [enable_rpath="yes"])



    case "x${enable_profile}" in

        xyes)
            ATHEME_TEST_LD_FLAGS([-pg])
            ;;

        xno)
            ;;

        *)
            AC_MSG_ERROR([invalid option for --enable-profile])
            ;;
    esac

    case "x${enable_relro}" in

        xyes)
            ATHEME_TEST_LD_FLAGS([-Wl,-z,relro])
            ;;

        xno)
            ;;

        *)
            AC_MSG_ERROR([invalid option for --enable-relro])
            ;;
    esac

    case "x${enable_nonlazy_bind}" in

        xyes)
            ATHEME_TEST_LD_FLAGS([-Wl,-z,now])
            ;;

        xno)
            ;;

        *)
            AC_MSG_ERROR([invalid option for --enable-nonlazy-bind])
            ;;
    esac

    case "x${enable_linker_defs}" in

        xyes)
            AS_IF([test "${COMPILER_SANITIZERS}" = "Yes"], [
                AC_MSG_ERROR([To use --enable-compiler-sanitizers you must pass --disable-linker-defs])
            ])
            ATHEME_TEST_LD_FLAGS([-Wl,-z,defs])
            ;;

        xno)
            ;;

        *)
            AC_MSG_ERROR([invalid option for --enable-linker-defs])
            ;;
    esac

    case "x${enable_noexecstack}" in

        xyes)
            ATHEME_TEST_LD_FLAGS([-Wl,-z,noexecstack])
            ;;

        xno)
            ;;

        *)
            AC_MSG_ERROR([invalid option for --enable-noexecstack])
            ;;
    esac

    case "x${enable_as_needed}" in

        xyes)
            ATHEME_TEST_LD_FLAGS([-Wl,--as-needed])
            ;;

        xno)
            ;;

        *)
            AC_MSG_ERROR([invalid option for --enable-as-needed])
            ;;
    esac

    case "x${enable_rpath}" in

        xyes)
            ATHEME_TEST_LD_FLAGS(${LDFLAGS_RPATH})
            ;;

        xno)
            ;;

        *)
            AC_MSG_ERROR([invalid option for --enable-rpath])
            ;;
    esac
])
