#include "atheme_perl.h"

SV * bless_pointer_to_package(void *data, const char *package)
{
	SV *ret = newSV(0);
	sv_setref_pv(ret, package, data);
	return ret;
}
