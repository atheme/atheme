SUBDIRS=$(LIBMOWGLI) include libathemecore modules src $(PODIR)
CLEANDIRS = ${SUBDIRS}
DISTCLEAN = extra.mk buildsys.mk config.log config.status atheme-services.pc

-include extra.mk
-include buildsys.mk

# explicit dependencies need to be expressed to ensure parallel builds don't die
libathemecore: include $(LIBMOWGLI)
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

dist:
	@if [ ! -d .hg ]; then \
		echo "make dist only works from a mercurial tree"; \
		false; \
	fi
	hg parent --template '#define SERNO "{rev}:{node|short}"\n' >include/serno.h
	@echo "Creating $(DISTNAME).tar.gz"
	$(RM) -f $(DISTNAME)
	$(LN) -s . $(DISTNAME)
	hg manifest | awk '{ print "$(DISTNAME)/"$$1; } END { print "$(DISTNAME)/configure"; print "$(DISTNAME)/aclocal.m4"; print "$(DISTNAME)/include/sysconf.h.in"; print "$(DISTNAME)/include/serno.h"; }' | $(TAR) -chnzf $(DISTNAME).tar.gz -T /dev/stdin
	$(RM) $(DISTNAME)

buildsys.mk:
	@echo "Run ./configure first you idiot."
	@exit 1
