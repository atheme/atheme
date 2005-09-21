# $Id$

&send("PASS papaya TS 6 :00T");
&send("CAPAB :QS EX CHW IE KLN GLN KNOCK TB UNKLN CLUSTER ENCAP SERVICES RSFNC");
&send("SERVER testsuite. 1 :testsuite server");
&send("SVINFO 6 3 ".time());
&send(":testsuite. PING testsuite. services.");

&waitFor(":services. PONG services. testsuite.");

1;
