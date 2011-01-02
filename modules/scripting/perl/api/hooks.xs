#include "perl_hooks.h"

MODULE = Atheme			PACKAGE = Atheme::Internal::Hooklist

void
enable_perl_hook_handler(const char *hookname)

void
disable_perl_hook_handler(const char *hookname)
