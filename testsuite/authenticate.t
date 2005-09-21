# $Id$

&send("PASS papaya :TS");
&send("SERVER testsuite. 1 :testsuite server");
&send("SVINFO 6 3 ".time());
&send(":testsuite. PING testsuite. services.");

&waitFor(":services. PONG services. testsuite.");

1;
