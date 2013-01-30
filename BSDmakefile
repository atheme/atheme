# This BSDmakefile redirects to GNU make.

all: .DEFAULT
.DEFAULT:
	@case ${MAKE} in \
	gmake|*/gmake) \
		echo "BSDmakefile run using gmake??"; \
		exit 1;; \
	esac
	gmake ${.MAKEFLAGS} ${.TARGETS}
