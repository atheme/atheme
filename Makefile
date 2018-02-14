SUBDIRS		= include libathemecore $(LIBMOWGLI_COND_D) modules src $(PODIR_COND_D)
CLEANDIRS	= ${SUBDIRS}
DISTCLEAN	= atheme-services.pc buildsys.mk config.log config.status extra.mk

-include extra.mk
-include buildsys.mk

# explicit dependencies need to be expressed to ensure parallel builds don't die
libathemecore: include $(LIBMOWGLI_COND_D)
modules: libathemecore
src: libathemecore

install-extra:
	@echo "----------------------------------------------------------------"
	@echo ">>> Remember to cd to ${prefix} and edit your config file.";
	@echo "----------------------------------------------------------------"
	i="atheme-services.pc"; \
	${INSTALL_STATUS}; \
	if ${MKDIR_P} ${DESTDIR}${libdir}/pkgconfig && ${INSTALL} -m 644 $$i ${DESTDIR}${libdir}/pkgconfig/$$i; then \
		${INSTALL_OK}; \
	else \
		${INSTALL_FAILED}; \
	fi

uninstall-extra:
	i="atheme-services.pc"; \
	if [ -f ${DESTDIR}${libdir}/pkgconfig/$$i ]; then \
		if rm -f ${DESTDIR}${libdir}/pkgconfig/$$i; then \
			${DELETE_OK}; \
		else \
			${DELETE_FAILED}; \
		fi \
	fi

buildsys.mk:
	@echo "Run ./configure first you idiot."
	@exit 1
