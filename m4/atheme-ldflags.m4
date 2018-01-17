ATHEME_LD_TEST_LDFLAGS_RESULT="no"

AC_DEFUN([ATHEME_LD_TEST_LDFLAGS],
	[
		AC_MSG_CHECKING([for C linker flag(s) $1 ])

		LDFLAGS_SAVED="${LDFLAGS}"
		LDFLAGS="${LDFLAGS} $1"

		AC_COMPILE_IFELSE(
			[
				AC_LANG_PROGRAM([[]], [[]])
			], [
				ATHEME_LD_TEST_LDFLAGS_RESULT='yes'

				AC_MSG_RESULT([yes])
			], [
				ATHEME_LD_TEST_LDFLAGS_RESULT='no'
				LDFLAGS="${LDFLAGS_SAVED}"

				AC_MSG_RESULT([no])
			], [
				ATHEME_LD_TEST_LDFLAGS_RESULT='no'
				LDFLAGS="${LDFLAGS_SAVED}"

				AC_MSG_RESULT([skipped as we are cross-compiling])
			]
		)

		unset LDFLAGS_SAVED
	]
)
