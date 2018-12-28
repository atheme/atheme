# Short KISS instructions:
#
# - To add new languages: Add *.po target to LOCALES Make variable below
# - Update and merge .po-files: make update-po
# - Only update the template .pot-file only: make update-pot
# - Run update-potfiles.sh when any new sourcefiles with translatable strings
#   have been introduced, and then continue with 'make update-po' or
#   'make update-pot' respectively.
#

LOCALES=bg.po cy.po da.po de.po es.po fr.po ru.po tr.po

include ../buildsys.mk

POTTEMPLATE=$(PACKAGE_TARNAME).pot

update-pot:
	@echo Updating $(POTTEMPLATE) ...
	xgettext --default-domain="$(PACKAGE_TARNAME)" --language=C \
	--keyword=_ --keyword=N_ --from-code="utf-8" --sort-output \
	--directory=.. --files-from=POTFILES.in -o "$(POTTEMPLATE)" \
	--package-name="$(PACKAGE_TARNAME)" --package-version="$(PACKAGE_VERSION)" \
	--copyright-holder="$(VENDOR_STRING)" --msgid-bugs-address="<$(PACKAGE_BUGREPORT)>"

update-po: update-pot
	@echo Updating .po -files ...
	@for lang in $(LOCALES); do echo -n "$$lang : "; msgmerge --backup=simple --update "$$lang" $(POTTEMPLATE); done
