# $Id$

&send("NICK testusr 1 ".time()." +io test test.net testsuite. :Test User");
&waitFor(":NickServ NOTICE testusr");

1;
