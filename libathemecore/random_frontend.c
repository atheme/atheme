/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2019 Aaron M. D. Jones <aaronmdjones@gmail.com>
 *
 * Frontend routines for the random interface.
 */

#include "sysconf.h"

#define ATHEME_RANDOM_FRONTEND_C 1

#if (ATHEME_RANDOM_FRONTEND == ATHEME_RANDOM_FRONTEND_INTERNAL)
#  include "random_fe_internal.c"
#else
#  if (ATHEME_RANDOM_FRONTEND == ATHEME_RANDOM_FRONTEND_OPENBSD)
#    include "random_fe_openbsd.c"
#  else
#    if (ATHEME_RANDOM_FRONTEND == ATHEME_RANDOM_FRONTEND_SODIUM)
#      include "random_fe_sodium.c"
#    else
#      if (ATHEME_RANDOM_FRONTEND == ATHEME_RANDOM_FRONTEND_MBEDTLS)
#        include "random_fe_mbedtls.c"
#      else
#        error "No RNG frontend"
#      endif
#    endif
#  endif
#endif
