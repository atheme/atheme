AC_DEFUN([ATHEME_FEATURETEST_HEAP_ALLOCATOR], [

	HEAP_ALLOCATOR="Yes"

	AC_ARG_ENABLE([heap-allocator],
		[AS_HELP_STRING([--disable-heap-allocator], [Disable heap allocator])],
		[], [enable_heap_allocator="yes"])

	case "x${enable_heap_allocator}" in
		xyes)
			HEAP_ALLOCATOR="Yes"
			;;
		xno)
			AC_DEFINE([DISABLE_HEAP_ALLOCATOR], [1], [Disable heap allocator])
			HEAP_ALLOCATOR="No"
			;;
		*)
			AC_MSG_ERROR([invalid option for --enable-heap-allocator])
			;;
	esac
])
