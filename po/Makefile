# SPDX-License-Identifier: CC0-1.0
# SPDX-URL: https://spdx.org/licenses/CC0-1.0.html
#
# Copyright (C) 2018-2020 Aaron M. D. Jones <me@aaronmdjones.net>
#
# Short KISS instructions:
#
# - To regenerate the list of translatable files, run:
#     `make -B update-potfiles'
#
# - To regenerate the master template atheme.pot, run:
#     `make -B update-pot'
#
# - To regenerate a translation file, run:
#     `make -B xx.po'
#     where xx.po is one of the files in the LOCALES variable below
#
# - To regenerate all of the translation files, run:
#     `make -B update-po'
#
# - To add a translation; add it to the LOCALES variable below
#
# - When adding a translation subdirectory to help/, also add it to the
#   HELP_LINGUAS variable in src/services/Makefile

include ../extra.mk

.PHONY: update-potfiles update-pot update-po
.SUFFIXES:

POTTEMPLATE=${PACKAGE_TARNAME}.pot

LOCALES=            \
    bg.po           \
    cy.po           \
    da.po           \
    de.po           \
    es.po           \
    fr.po           \
    ru.po           \
    tr.po

include ../buildsys.mk

update-potfiles:
	@echo "Updating POTFILES.in ..."
	@sh update-potfiles.sh

update-pot: update-potfiles
	@echo "Updating ${POTTEMPLATE} ..."
	@xgettext                                               \
	    --default-domain="${PACKAGE_TARNAME}"               \
	    --package-name="${PACKAGE_NAME}"                    \
	    --package-version="${PACKAGE_VERSION}"              \
	    --copyright-holder="${VENDOR_STRING}"               \
	    --msgid-bugs-address="<${PACKAGE_BUGREPORT_I18N}>"  \
	    --add-comments="TRANSLATORS"                        \
	    --directory=".."                                    \
	    --files-from="POTFILES.in"                          \
	    --from-code="utf-8"                                 \
	    --keyword="_"                                       \
	    --keyword="N_"                                      \
	    --language=C                                        \
	    --escape                                            \
	    --foreign-user                                      \
	    --sort-output                                       \
	    -o "${POTTEMPLATE}.tmp"
	@cat < "${POTTEMPLATE}.header" > "${POTTEMPLATE}"
	@tail -n +4 < "${POTTEMPLATE}.tmp" >> "${POTTEMPLATE}"
	@rm -f "${POTTEMPLATE}.tmp"

${LOCALES}:
	@echo "Updating $@ ..."
	@msgmerge                       \
	    --backup=none               \
	    --quiet                     \
	    --sort-output               \
	    --update                    \
	    "$@" "${POTTEMPLATE}"

update-po: update-pot ${LOCALES}
