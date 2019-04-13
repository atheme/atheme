# Short KISS instructions:
#
# - To regenerate the list of translatable files; run `make update-potfiles'
# - To regenerate the master template atheme.pot; run `make update-pot'
# - To regenerate all the translation files; run `make update-po'
# - To add a translation; add it to LOCALES below, and to the LINGUAS file
# - When adding a translation subdirectory to help/, also add it to the
#   HELP_LINGUAS variable in src/services/Makefile

LOCALES=bg.po cy.po da.po de.po es.po fr.po ru.po tr.po

include ../buildsys.mk

POTTEMPLATE=$(PACKAGE_TARNAME).pot

update-potfiles:
	@echo "Updating POTFILES.in ..."
	@sh update-potfiles.sh

update-pot: update-potfiles
	@echo "Updating $(POTTEMPLATE) ..."
	xgettext                                                \
	    --default-domain="$(PACKAGE_TARNAME)"               \
	    --package-name="$(PACKAGE_NAME)"                    \
	    --package-version="$(PACKAGE_VERSION)"              \
	    --copyright-holder="$(VENDOR_STRING)"               \
	    --msgid-bugs-address="<$(PACKAGE_BUGREPORT)>"       \
	    --add-comments="TRANSLATORS"                        \
	    --directory=".."                                    \
	    --files-from="POTFILES.in"                          \
	    --from-code="utf-8"                                 \
	    --keyword="_"                                       \
	    --keyword="N_"                                      \
	    --language=C                                        \
	    --foreign-user                                      \
	    --sort-output                                       \
	    --width=116                                         \
	    -o "$(POTTEMPLATE).tmp"
	cat < "$(POTTEMPLATE).header" > "$(POTTEMPLATE)"
	tail -n +4 < "$(POTTEMPLATE).tmp" >> "$(POTTEMPLATE)"
	rm -f "$(POTTEMPLATE).tmp"

update-po: update-pot
	@for lang in $(LOCALES) ; do echo "Updating $$lang ..." ; msgmerge --backup=none --quiet --sort-output --update --width=116 "$$lang" $(POTTEMPLATE) ; done
