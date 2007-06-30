include ../../../mk/rules.mk
include ../../../mk/init.mk
include ../../../mk/objective.mk

OBJECTIVE_BINS = randomtest

LDADD = -L../../libmowgli -lmowgli

CFLAGS += -I../../libmowgli

SOURCES = randomtest.c

OBJECTS = ${SOURCES:.c=.o}

randomtest: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LDADD) -o $@
	@printf "%10s     %-20s\n" LINK $@
