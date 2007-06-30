include ../../mk/rules.mk
include ../../mk/init.mk

PICLDFLAGS = $(LIBLDFLAGS)

OBJECTIVE_LIBS = libmowgli$(SHARED_SUFFIX)
OBJECTIVE_SONAME_SUFFIX = 1

LIBADD += $(DYNAMIC_LD_LIBS)

CFLAGS += $(PICFLAGS) -I.. -DMOWGLI_CORE

SOURCES = \
	mowgli_alloc.c \
	mowgli_argstack.c \
	mowgli_bitvector.c \
	mowgli_dictionary.c \
	mowgli_error_backtrace.c \
	mowgli_formatter.c \
	mowgli_global_storage.c \
	mowgli_hash.c \
	mowgli_heap.c \
	mowgli_hook.c \
	mowgli_init.c \
	mowgli_ioevent.c \
	mowgli_list.c \
	mowgli_logger.c \
	mowgli_mempool.c \
	mowgli_module.c \
	mowgli_object.c \
	mowgli_object_class.c \
	mowgli_object_messaging.c \
	mowgli_object_metadata.c \
	mowgli_queue.c \
	mowgli_random.c \
	mowgli_signal.c \
	mowgli_spinlock.c \
	mowgli_string.c

OBJECTS = ${SOURCES:.c=.o}

INCLUDEDIR = ${prefix}/include/libmowgli

HEADERS = \
	mowgli.h \
	mowgli_alloc.h \
	mowgli_argstack.h \
	mowgli_assert.h \
	mowgli_bitvector.h \
	mowgli_config.h \
	mowgli_dictionary.h \
	mowgli_error_backtrace.h \
	mowgli_exception.h \
	mowgli_formatter.h \
	mowgli_global_storage.h \
	mowgli_hash.h \
	mowgli_heap.h \
	mowgli_hook.h \
	mowgli_init.h \
	mowgli_ioevent.h \
	mowgli_list.h \
	mowgli_logger.h \
	mowgli_mempool.h \
	mowgli_module.h \
	mowgli_object.h \
	mowgli_object_class.h \
	mowgli_object_messaging.h \
	mowgli_object_metadata.h \
	mowgli_queue.h \
	mowgli_random.h \
	mowgli_signal.h \
	mowgli_spinlock.h \
	mowgli_stdinc.h \
	mowgli_string.h

include ../../mk/objective.mk

install-posthook:
	@mv ${DESTDIR}/${LIBDIR}/libmowgli$(SHARED_SUFFIX) ${DESTDIR}/${LIBDIR}/libmowgli$(SHARED_SUFFIX).1.0.0
	@ln -sf libmowgli$(SHARED_SUFFIX).1.0.0 \
		${DESTDIR}/${LIBDIR}/libmowgli$(SHARED_SUFFIX).1
	@ln -sf libmowgli$(SHARED_SUFFIX).1 \
		${DESTDIR}/${LIBDIR}/libmowgli$(SHARED_SUFFIX)
