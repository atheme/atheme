# $Id$

$chtime = time();

# Register the channel.
&send(":testsuite. SJOIN $chtime #testchan +nt :@testusr");
&send(":testusr PRIVMSG ChanServ :REGISTER #testchan");

&waitFor(":ChanServ NOTICE testusr :\2#testchan\2 is now registered to \2testusr\2");

&send(":testusr PART #testchan");
&send(":testsuite. SJOIN $chtime #testchan +nt :@testusr");
&waitFor(":services. SJOIN $chtime #testchan + :@ChanServ");

&send(":testusr PRIVMSG ChanServ :INFO #testchan");
&waitFor(":ChanServ NOTICE testusr :Information on \2#testchan\2");

1;
