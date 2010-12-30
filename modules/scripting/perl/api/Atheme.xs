#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>

/* Take the Atheme definition of this. */
#undef _

#include "atheme.h"
#include "atheme_perl.h"

typedef sourceinfo_t *Atheme_Sourceinfo;
typedef perl_command_t *Atheme_Command;
typedef service_t *Atheme_Service;


MODULE = Atheme			PACKAGE = Atheme

INCLUDE: services.xs
INCLUDE: sourceinfo.xs
INCLUDE: commands.xs
