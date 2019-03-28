#!/bin/sh

rm -f POTFILES*
echo "# Please don't update this file manually - use 'make update-potfiles' instead!" > POTFILES.in
cd ..
find libathemecore/ modules/ src/ \( -name "*.c" -o -name "*.cxx" -o -name "*.cc" -o -name "*.glade" \) -exec grep -lE "translatable|_\(" \{\} \+ | sort | uniq >> po/POTFILES.in
