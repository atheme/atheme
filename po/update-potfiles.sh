#!/bin/sh

rm -f POTFILES*
echo "# Please don't update this file manually - use 'make update-potfiles' instead!" > POTFILES.in
cd ..

echo 'include/atheme/constants.h' >> po/POTFILES.in
find libathemecore/ modules/ src/ \( -name "*.c" -o -name "*.cxx" -o -name "*.cc" -o -name "*.glade" \) -exec grep -lE "translatable|_\(" \{\} \+ | sort | uniq >> po/POTFILES.in
