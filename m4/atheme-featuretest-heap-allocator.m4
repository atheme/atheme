AC_DEFUN([ATHEME_FEATURETEST_HEAP_ALLOCATOR], [

    HEAP_ALLOCATOR="Yes"

    AC_ARG_ENABLE([heap-allocator],
        [AS_HELP_STRING([--disable-heap-allocator], [Disable the heap allocator])],
        [], [enable_heap_allocator="yes"])

    case "x${enable_heap_allocator}" in
        xyes)
            HEAP_ALLOCATOR="Yes"
            AC_DEFINE([ATHEME_ENABLE_HEAP_ALLOCATOR], [1], [Define to 1 if --disable-heap-allocator was NOT given to ./configure])
            ;;
        xno)
            HEAP_ALLOCATOR="No"
            ;;
        *)
            AC_MSG_ERROR([invalid option for --enable-heap-allocator])
            ;;
    esac
])
