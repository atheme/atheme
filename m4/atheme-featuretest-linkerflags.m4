AC_DEFUN([ATHEME_FEATURETEST_LINKERFLAGS], [

	AC_ARG_ENABLE([profile],
		[AS_HELP_STRING([--enable-profile], [Enable profiling extensions])],
		[], [enable_profile="no"])

	AC_ARG_ENABLE([relro],
		[AS_HELP_STRING([--disable-relro], [Disable -Wl,-z,relro (marks the relocation table read-only)])],
		[], [enable_relro="yes"])

	AC_ARG_ENABLE([nonlazy-bind],
		[AS_HELP_STRING([--disable-nonlazy-bind], [Disable -Wl,-z,now (resolves all symbols at program startup time)])],
		[], [enable_nonlazy_bind="yes"])

	AC_ARG_ENABLE([as-needed],
		[AS_HELP_STRING([--disable-as-needed], [Disable -Wl,--as-needed (strips unnecessary libraries at link time)])],
		[], [enable_as_needed="yes"])

	AC_ARG_ENABLE([rpath],
		[AS_HELP_STRING([--disable-rpath], [Disable -Wl,-rpath= (builds installation path into binaries)])],
		[], [enable_rpath="yes"])

	case "${enable_profile}" in
		yes)
			ATHEME_LD_TEST_LDFLAGS([-pg])
			;;
		no)
			;;
		*)
			AC_MSG_ERROR([invalid option for --enable-profile])
			;;
	esac

	case "${enable_relro}" in
		yes)
			ATHEME_LD_TEST_LDFLAGS([-Wl,-z,relro])
			;;
		no)
			;;
		*)
			AC_MSG_ERROR([invalid option for --enable-relro])
			;;
	esac

	case "${enable_nonlazy_bind}" in
		yes)
			ATHEME_LD_TEST_LDFLAGS([-Wl,-z,now])
			;;
		no)
			;;
		*)
			AC_MSG_ERROR([invalid option for --enable-nonlazy-bind])
			;;
	esac

	case "${enable_as_needed}" in
		yes)
			ATHEME_LD_TEST_LDFLAGS([-Wl,--as-needed])
			;;
		no)
			;;
		*)
			AC_MSG_ERROR([invalid option for --enable-as-needed])
			;;
	esac

	case "${enable_rpath}" in
		yes)
			ATHEME_LD_TEST_LDFLAGS(${LDFLAGS_RPATH})
			;;
		no)
			;;
		*)
			AC_MSG_ERROR([invalid option for --enable-rpath])
			;;
	esac
])
