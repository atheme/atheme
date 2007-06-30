include ../../../mk/rules.mk
include ../../../mk/init.mk
include ../../../mk/objective.mk

OBJECTIVE_BINS = formattertest

LDADD = -L../../libmowgli -lmowgli

CFLAGS += -I../../libmowgli

SOURCES = formattertest.c

OBJECTS = ${SOURCES:.c=.o}

formattertest: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LDADD) -o $@
	@printf "%10s     %-20s\n" LINK $@
