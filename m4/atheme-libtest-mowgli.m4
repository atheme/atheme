AC_DEFUN([ATHEME_LIBTEST_MOWGLI], [

	MOWGLI_SOURCE=""

	AC_ARG_WITH([libmowgli],
		[AS_HELP_STRING([--with-libmowgli@<:@=prefix@:>@], [Specify location of system libmowgli install, "yes" to ask pkg-config (default), or "no" to force use of internal libmowgli submodule (fallback)])],
		[], [with_libmowgli="yes"])

	AS_IF([test "x${with_libmowgli}" = "xyes"], [
		PKG_CHECK_MODULES([MOWGLI], [libmowgli-2 >= 2.0.0], [
			MOWGLI_SOURCE="System"
			CPPFLAGS="${CPPFLAGS} ${MOWGLI_CFLAGS}"
			LIBS="${LIBS} ${MOWGLI_LIBS}"
		], [
			with_libmowgli="no"
		])
	])

	AS_IF([test "x${with_libmowgli}" = "xyes"], [], [test "x${with_libmowgli}" = "xno"], [
		MOWGLI_SOURCE="Internal"
		LIBMOWGLI_COND_D="libmowgli-2"
		AC_SUBST([LIBMOWGLI_COND_D])
		CPPFLAGS="${CPPFLAGS} -I$(pwd)/libmowgli-2/src/libmowgli"
		LDFLAGS="${LDFLAGS} -L$(pwd)/libmowgli-2/src/libmowgli"
		LIBS="${LIBS} -lmowgli-2"
	], [
		MOWGLI_SOURCE="System"
		CPPFLAGS="${CPPFLAGS} -I${with_libmowgli}/include/libmowgli-2"
		LDFLAGS="${LDFLAGS} -L${with_libmowgli}/lib"
		LIBS="${LIBS} -lmowgli-2"
	])
])
