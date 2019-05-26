AC_DEFUN([ATHEME_LIBTEST_RT], [

	LIBRT_LIBS=""

	AC_SEARCH_LIBS([clock_gettime], [rt], [
		AS_IF([test "x${ac_cv_search_clock_gettime}" != "xnone required"], [
			LIBRT_LIBS="${ac_cv_search_clock_gettime}"
		])

		AC_SUBST([LIBRT_LIBS])
		ATHEME_COND_PBKDF2_BENCHMARK_ENABLE
	], [], [])
])
