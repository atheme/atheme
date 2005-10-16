/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Win32 special functions
 *
 * $Id$
 */

#include "atheme.h"

#ifndef _WIN32
#error "Why is win32.c being compiled on a non-windows system? Ease on the crack, please."
#else
#warning "You are using a crappy OS. Upgrade to something better! Ok, fine, we'll do this crazy thing anyway."
#endif

/* Undefine our gethostbyname() macro to stop a likely horrible death */
#undef gethostbyname
/*typedef struct hostent {
  char FAR* h_name;
  char FAR  FAR** h_aliases;
  short h_addrtype;
  short h_length;
  char FAR  FAR** h_addr_list;
} hostent;*/
/* Define a localhost hostent. */
struct hostent w32_hostent_local = {
	0, 0, 0, 0, 0,
};

/* Implements a gethostbyname that can understand IP addresses */
struct hostent *FAR gethostbyname_layer( const char* name )
{
	struct hostent *hp;
	unsigned long addr;
	
	if ( ( hp = gethostbyname( name ) ) != NULL )
	{
		/* We were given a real host, so we're fine. */
		return hp;
	}
	
	printf( "Attempting to resolve %s\n", name );
	
	/* We might have an IP address that Windows couldn't handle -- try. */
	
	addr = inet_addr( name );
	
	printf( "  Address: %ul\n", addr );
	
	hp = gethostbyaddr( (char *)&addr, 4, AF_INET );
	
	printf( "  hostent: %p\n", hp );
	
	if ( hp == 0 )
	{
		printf( "Windows sucks because: %d\n", WSAGetLastError( ) );
	}
	
	return hp;
}
