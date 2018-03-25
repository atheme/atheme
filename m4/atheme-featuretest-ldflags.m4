ATHEME_LD_TEST_LDFLAGS_RESULT="no"

AC_DEFUN([ATHEME_LD_TEST_LDFLAGS],
	[
		AC_MSG_CHECKING([for C linker flag(s) $1 ])

		LDFLAGS_SAVED="${LDFLAGS}"
		LDFLAGS="${LDFLAGS} $1"

		AC_LINK_IFELSE(
			[
				AC_LANG_PROGRAM([[]], [[]])
			], [
				ATHEME_LD_TEST_LDFLAGS_RESULT='yes'

				AC_MSG_RESULT([yes])
			], [
				ATHEME_LD_TEST_LDFLAGS_RESULT='no'
				LDFLAGS="${LDFLAGS_SAVED}"

				AC_MSG_RESULT([no])
			]
		)

		unset LDFLAGS_SAVED
	]
)

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

	case "${enable_linker_defs}" in
		yes)
			ATHEME_LD_TEST_LDFLAGS([-Wl,-z,defs])
			;;
		no)
			;;
		*)
			AC_MSG_ERROR([invalid option for --enable-linker-defs])
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
