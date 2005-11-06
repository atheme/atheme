/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Win32 special functions
 *
 * $Id: win32.c 3607 2005-11-06 23:57:17Z jilles $
 */

#include <org.atheme.claro.base>

/* Undefine our gethostbyname() macro to stop a likely horrible death */
#undef gethostbyname

/* Define a localhost hostent. */
struct hostent w32_hostent_local = {
	0, 0, 0, 0, 0,
};

/* Implements a gethostbyname that can understand IP addresses */
struct hostent *FAR gethostbyname_layer( const char* name )
{
	struct hostent *hp;
	unsigned long addr;
	
	if ((hp = gethostbyname(name)) != NULL)
	{
		/* We were given a real host, so we're fine. */
		return hp;
	}
	
	printf( "Attempting to resolve %s\n", name );
	
	/* We might have an IP address that Windows couldn't handle -- try. */	
	addr = inet_addr(name);
	hp = gethostbyaddr((char *)&addr, 4, AF_INET);
	
	if (hp == NULL)
	{
		printf("Windows sucks because: %d\n", WSAGetLastError());
	}
	
	return hp;
}
