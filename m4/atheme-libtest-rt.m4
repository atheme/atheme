AC_DEFUN([ATHEME_LIBTEST_RT], [

	LIBRT="No"
	LIBRT_LIBS=""

	LIBS_SAVED="${LIBS}"

	AC_SEARCH_LIBS([clock_gettime], [rt], [
		AS_IF([test "x${ac_cv_search_clock_gettime}" != "xnone required"], [
			LIBRT_LIBS="${ac_cv_search_clock_gettime}"
		])

		LIBRT="Yes"
		AC_SUBST([LIBRT_LIBS])
		ATHEME_COND_CRYPTO_BENCHMARK_ENABLE
	], [], [])

	LIBS="${LIBS_SAVED}"
])
