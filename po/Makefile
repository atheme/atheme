# Short KISS instructions:
#
# - To add new languages: Add *.po target to LOCALES Make variable below
# - Update and merge .po-files: make update-po
# - Only update the template .pot-file only: make update-pot
# - Run update-potfiles.sh when any new sourcefiles with translatable strings
#   have been introduced, and then continue with 'make update-po' or
#   'make update-pot' respectively.
#

LOCALES=es.po ru.po cy.po de.po da.po fr.po bg.po tr.po

include ../buildsys.mk

POTTEMPLATE=$(PACKAGE_NAME).pot

update-pot:
	@echo Updating $(POTTEMPLATE) ...
	xgettext --default-domain=$(PACKAGE_NAME) --language=C \
	--keyword=_ --keyword=N_ --from-code="utf-8" \
	--directory=.. --files-from=POTFILES.in -o $(POTTEMPLATE) \
	--package-name=$(PACKAGE_NAME) --package-version=$(PACKAGE_VERSION) \
	--copyright-holder=Atheme \
	--msgid-bugs-address=https://github.com/atheme/atheme/issues

update-po: update-pot
	@echo Updating .po -files ...
	@for lang in $(LOCALES); do echo -n "$$lang : "; msgmerge --backup=simple --update "$$lang" $(POTTEMPLATE); done
