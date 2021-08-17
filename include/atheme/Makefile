# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2012 William Pitcock <nenolod@dereferenced.org>
# Copyright (C) 2012 Jilles Tjoelker <jilles@stack.nl>
# Copyright (C) 2014 Rylai <gando.depth@gmail.com>
# Copyright (C) 2018-2020 Aaron M. D. Jones <me@aaronmdjones.net>

.PHONY: serno.h

include ../../extra.mk

DISTCLEAN = hooktypes.h instpaths.h serno.h sysconf.h
SUBDIRS   = digest inline protocol

includesubdir = atheme

INCLUDES =                  \
    abirev.h                \
    account.h               \
    argon2.h                \
    attributes.h            \
    auth.h                  \
    authcookie.h            \
    base64.h                \
    bcrypt.h                \
    botserv.h               \
    channels.h              \
    commandhelp.h           \
    commandtree.h           \
    common.h                \
    conf.h                  \
    confprocess.h           \
    connection.h            \
    constants.h             \
    crypto.h                \
    culture.h               \
    database_backend.h      \
    datastream.h            \
    digest.h                \
    email.h                 \
    entity-validation.h     \
    entity.h                \
    flags.h                 \
    global.h                \
    hook.h                  \
    hooktypes.h             \
    httpd.h                 \
    i18n.h                  \
    inline.h                \
    instpaths.h             \
    libathemecore.h         \
    linker.h                \
    match.h                 \
    memory.h                \
    module.h                \
    object.h                \
    pbkdf2.h                \
    phandler.h              \
    pmodule.h               \
    privs.h                 \
    random.h                \
    sasl.h                  \
    scrypt.h                \
    serno.h                 \
    servers.h               \
    services.h              \
    servtree.h              \
    sharedheap.h            \
    sourceinfo.h            \
    stdheaders.h            \
    string.h                \
    structures.h            \
    sysconf.h               \
    table.h                 \
    taint.h                 \
    template.h              \
    tools.h                 \
    uid.h                   \
    uplink.h                \
    users.h

pre-depend: ${DISTCLEAN}

hooktypes.h: hooktypes.in mkhooktypes.sh
	$(info Generate: $@)
	@sh mkhooktypes.sh hooktypes.in >$@

instpaths.h: ../../extra.mk instpaths.h.in
	$(info Generate: $@)
	@$(SED)                                         \
		-e 's|@BINDIR[@]|$(bindir)|g'           \
		-e 's|@DATADIR[@]|$(DATADIR)|g'         \
		-e 's|@DOCDIR[@]|$(DOCDIR)|g'           \
		-e 's|@LOCALEDIR[@]|$(LOCALEDIR)|g'     \
		-e 's|@LOGDIR[@]|$(LOGDIR)|g'           \
		-e 's|@MODDIR[@]|$(MODDIR)|g'           \
		-e 's|@PREFIX[@]|$(prefix)|g'           \
		-e 's|@RUNDIR[@]|$(RUNDIR)|g'           \
		-e 's|@SHAREDIR[@]|$(SHAREDIR)|g'       \
		-e 's|@SYSCONFDIR[@]|$(sysconfdir)|g'   \
	< "$@.in" > "$@"

serno.h:
	@sh mkserno.sh

include ../../buildsys.mk
