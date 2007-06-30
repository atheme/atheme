include ../../../mk/rules.mk
include ../../../mk/init.mk
include ../../../mk/objective.mk

OBJECTIVE_BINS = listsort

LDADD = -L../../libmowgli -lmowgli

CFLAGS += -I../../libmowgli

SOURCES = listsort.c

OBJECTS = ${SOURCES:.c=.o}

listsort: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LDADD) -o $@
	@printf "%10s     %-20s\n" LINK $@
