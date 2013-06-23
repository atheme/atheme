MODULE = Atheme			PACKAGE = Atheme::Config

unsigned int
default_channel_flags()
CODE:
    RETVAL = config_options.defcflags;
OUTPUT:
    RETVAL

MODULE = Atheme			PACKAGE = Atheme::ChanServ::Config
#include "../../../chanserv/chanserv.h"

char *
default_templates ()
CODE:
    RETVAL = chansvs.deftemplates;
OUTPUT:
    RETVAL

unsigned int
founder_flags ()
CODE:
    RETVAL = custom_founder_check();
OUTPUT:
    RETVAL
