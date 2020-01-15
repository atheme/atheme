SUBDIRS		= $(LIBMOWGLI_COND_D) include libathemecore modules src $(PODIR_COND_D)
CLEANDIRS	= ${SUBDIRS}
DISTCLEAN	= buildsys.mk config.log config.status extra.mk

-include extra.mk
-include buildsys.mk

# explicit dependencies need to be expressed to ensure parallel builds don't die
libathemecore: $(LIBMOWGLI_COND_D) include
modules: libathemecore
src: libathemecore

install-extra:
	@echo "----------------------------------------------------------------"
	@echo ">>> Remember to cd to ${prefix} and edit your config file.";
	@echo "----------------------------------------------------------------"

buildsys.mk:
	@echo "Run ./configure first you idiot."
	@exit 1
