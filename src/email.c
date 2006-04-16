/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains email-related routines.
 *
 * $Id: email.c 5097 2006-04-16 02:45:02Z w00t $
 */

#include "atheme.h"

email_t *email_create(const char *sender, const char *reciever, const char *subject, const char *body, const char **headers)
{
	
}

boolean_t email_send(email_t *email)
{
	
}

boolean_t email_destroy(email_t *email)
{
	
}

#if 0

/* suggested callback for modules sending mail */
void cb_on_emailsend(email_t *email)
{
	
}

#endif


