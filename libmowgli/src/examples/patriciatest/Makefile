include ../../../mk/rules.mk
include ../../../mk/init.mk
include ../../../mk/objective.mk

OBJECTIVE_BINS = patriciatest

LDADD = -L../../libmowgli -lmowgli

CFLAGS += -I../../libmowgli

SOURCES = patriciatest.c

OBJECTS = ${SOURCES:.c=.o}

patriciatest: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LDADD) -o $@
	@printf "%10s     %-20s\n" LINK $@
