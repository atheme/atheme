# Additional extensions for building single-file modules.

.SUFFIXES: .so
PLUGIN=${SRCS:.c=.so}

.c.so:
	${COMPILE_STATUS}
	if ${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $< ${PLUGIN_LDFLAGS} ${LDFLAGS} ${LIBS}; then \
		${COMPILE_OK}; \
	else \
		${COMPILE_FAILED}; \
	fi

COMPILE_OK = printf "\033[K\033[0;32mSuccessfully compiled \033[1;32m$<\033[0;32m as \033[1;32m$@\033[0;32m.\033[0m\n"
