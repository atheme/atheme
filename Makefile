-include extra.mk
-include buildsys.mk

SUBDIRS=$(LIBMOWGLI) modules src po
CLEANDIRS = ${SUBDIRS}

pre-depend: include/hooktypes.h
	@if [ -d .hg ] ; then \
		revh=`hg tip --template '#define SERNO "#rev#:#node|short#"\n' 2>/dev/null`;\
		[ -z "$$revh" ] || echo "$$revh" >include/serno.h ; \
	fi

install-extra:
	@echo "----------------------------------------------------------------"
	@echo ">>> Remember to cd to ${prefix} and edit your config file.";
	@echo "----------------------------------------------------------------"

dist:
	@if [ ! -d .hg ]; then \
		echo "make dist only works from a mercurial tree"; \
		false; \
	fi
	hg tip --template '#define SERNO "#rev#:#node|short#"\n' >include/serno.h
	@echo "Creating $(DISTNAME).tar.gz"
	$(RM) -f $(DISTNAME)
	$(LN) -s . $(DISTNAME)
	hg manifest | awk '{ print "$(DISTNAME)/"$$1; } END { print "$(DISTNAME)/configure"; print "$(DISTNAME)/aclocal.m4"; print "$(DISTNAME)/include/sysconf.h.in"; print "$(DISTNAME)/include/serno.h"; }' | $(TAR) -chnzf $(DISTNAME).tar.gz -T /dev/stdin
	$(RM) $(DISTNAME)

include/hooktypes.h: ${SRCDIR}/src/mkhooktypes.sh ${SRCDIR}/src/hooktypes.in
	(cd src && touch .depend && ${MAKE} ../include/hooktypes.h)

buildsys.mk:
	@echo "Run ./configure first you idiot."
	@exit 1
