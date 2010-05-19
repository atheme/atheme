# Short KISS instructions:
#
# - To add new languages: Add *.po target to LOCALES Make variable below
# - Update and merge .po-files: make update-po
# - Only update the template .pot-file only: make update-pot
# - Run update-potfiles.sh when any new sourcefiles with translatable strings
#   have been introduced, and then continue with 'make update-po' or
#   'make update-pot' respectively.
#

LOCALES=es.po ru.po

include ../buildsys.mk

POTTEMPLATE=$(PACKAGE).pot

update-pot:
	@echo Updating $(POTTEMPLATE) ...
	xgettext --default-domain=$(PACKAGE) --language=C \
	--keyword=_ --keyword=N_ --from-code="utf-8" \
	--directory=.. --files-from=POTFILES.in -o $(POTTEMPLATE)

update-po: update-pot
	@echo Updating .po -files ...
	@for lang in $(LOCALES); do echo -n "$$lang : "; msgmerge --backup=simple --update "$$lang" $(POTTEMPLATE); done
