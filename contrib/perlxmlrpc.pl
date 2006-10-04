#!/usr/bin/perl -w
# Simple example of using Atheme's XMLRPC server with perl RPC::XML.
# $Id: perlxmlrpc.pl 6651 2006-10-04 13:56:08Z jilles $

require RPC::XML;
require RPC::XML::Client;

my $cli = RPC::XML::Client->new('http://127.0.0.1:8080/atheme');
my $resp;
my $authcookie = undef;
my $login;
my $password;

open(TTY, "+</dev/tty") or die "open /dev/tty: $!";
print TTY "login: ";
$login = <TTY>;
exit 0 if ($login eq '');
system("stty -echo </dev/tty");
print TTY "Password:";
$password = <TTY>;
system("stty echo </dev/tty");
print TTY "\n";
exit 0 if ($password eq '');

$resp = $cli->simple_request('atheme.login', $login, $password);
if (defined $resp)
{
	if (ref $resp)
	{
		print "Response(ref):";
		print " ", $_, "=", $resp->{$_} foreach keys %$resp;
		print "\n";
	}
	else
	{
		print "Response: $resp\n";
		$authcookie = $resp;
	}
}
else
{
	print "Failed: ", $RPC::XML::ERROR, "\n";
}

if (defined $authcookie)
{
	$resp = $cli->simple_request('atheme.command', $authcookie, $login, '.', 'ChanServ', 'DEOP', '#irc', 'jilles');
	#$resp = $cli->simple_request('atheme.command', $authcookie, $login, '.', 'ChanServ', 'KICK', '#irc', 'jilles', 'xmlrpc test');
	#$resp = $cli->simple_request('atheme.command', $authcookie, $login, '.', 'ChanServ', 'INFO', '#irc');
	#$resp = $cli->simple_request('atheme.command', $authcookie, $login, '.', 'ChanServ', 'FLAGS', '#irc');
	if (defined $resp)
	{
		if (ref $resp)
		{
			print "Response(ref):";
			print " ", $_, "=", $resp->{$_} foreach keys %$resp;
			print "\n";
		}
		else
		{
			print "Response: $resp\n";
		}
	}
	else
	{
		print "Failed: ", $RPC::XML::ERROR, "\n";
	}
	$resp = $cli->simple_request('atheme.logout', $authcookie, $login);
}
