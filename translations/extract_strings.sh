#!/bin/sh
# Copyright (c) 2006 Jilles Tjoelker
# Rights to this code are as documented in doc/LICENSE.
#
# Script to extract message strings.
# Execute this from translations/.

(cd ..; sed -ne 's/^.*[[:<:]]notice([^,]*,[^,]*,[^,]*"\([^"]*\)"[,)].*$/\1/p
s/^#define STR_[^"]*"\(.*\)".*$/\1/p' `find . -name '*.[ch]'`) |
	sort | uniq >atheme.strings

(echo "/* \$"'Id$ */';
echo 'language {'
echo '  name = "Everything";'
echo '  translator = "extract_strings.sh";'
echo '};'
echo
sed -e 's/^.*$/string "&" {\
  translation = "&";\
};/' atheme.strings
) > atheme.language
