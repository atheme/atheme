# Copyright (c) 2003-2004 E. Will et al.
# Rights to this code are documented in doc/LICENSE.
#
# This file contains build instructions.
#
# $Id:$
#

#send.c		\
#read.c		\
#list.c		\
#forward.c	\
#ignore.c	\
#delete.c	

CC		= gcc
RM		= /bin/rm
MV		= /bin/mv
CP		= /bin/cp
INSTALL		= /usr/bin/install -c
PREFIX		= /home/gnetdev/atheme
BIN		= atheme
MKDEP		= gcc -MM -DPREFIX=\"/home/gnetdev/atheme\" -I../../include -I/usr/local/include -I/usr/local/include/mysql
PICFLAGS	= -fPIC -DPIC -shared
CFLAGS		= -g -O2   -I../../include -I/usr/local/include -I/usr/local/include/mysql

VERSION		= 0.1

LIBS		=  -lpq
LDFLAGS		= -L/usr/local/lib -Wl,-export-dynamic
CPPFLAGS	= 

default: all

BASE_SRCS = 		\
	main.c		\
	help.c		\
	send.c		\
	list.c		\
	
SRCS = ${BASE_SRCS}

OBJS = ${SRCS:.c=.so}

all: modules

build: all

install: build
	${INSTALL} -d ${PREFIX}/modules/memoserv
	${INSTALL} -m 755 *.so ${PREFIX}/modules/memoserv

modules: $(OBJS)

.SUFFIXES: .so

.c.so:
	${CC} ${PICFLAGS} ${CPPFLAGS} ${CFLAGS} $< -o $@

.PHONY: clean distclean

clean:
	${RM} -f *.so

distclean: clean
	${RM} -f Makefile version.c.last
