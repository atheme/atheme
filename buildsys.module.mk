# Additional extensions for building single-file modules.

.SUFFIXES: $(PLUGIN_SUFFIX)

plugindir = ${MODDIR}/modules/$(MODULE)
PLUGIN=${SRCS:.c=$(PLUGIN_SUFFIX)}

.c$(PLUGIN_SUFFIX):
	${COMPILE_STATUS}
	if ${CC} ${PLUGIN_CFLAGS} ${PLUGIN_LDFLAGS} ${CFLAGS} ${CPPFLAGS} ${LIBS} -o $@ $<; then \
		${COMPILE_OK}; \
	else \
		${COMPILE_FAILED}; \
	fi

COMPILE_OK = printf "\033[K\033[0;32mSuccessfully compiled \033[1;32m$<\033[0;32m as \033[1;32m$@\033[0;32m.\033[0m\n"
