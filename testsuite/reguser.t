# $Id$

&send(":testusr PRIVMSG NickServ :REGISTER testpass testuser\@test.net");
&waitFor(":NickServ PRIVMSG testusr :\2testusr\2 is now registered to \2testuser\@test.net\2");

&send(":testusr QUIT :testing onconnect");
&send("NICK testusr 1 ".time()." +io testusr test.net testsuite. :Test User");
&waitFor(":NickServ PRIVMSG testusr :This nickname is registered.");

&send(":testusr PRIVMSG NickServ :IDENTIFY testpass");
&waitFor(":NickServ PRIVMSG testusr :You are now identified for \2testusr\2.");

&send(":testusr PRIVMSG NickServ :ACC");
&waitFor(":NickServ PRIVMSG testusr :testusr ACC 3");

1;
