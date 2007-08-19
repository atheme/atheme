#!/usr/bin/perl -w
# A simple CGI script for the XMLRPC interface.
# Allows users to log in and view their user information (/ns info).

use CGI;

$CGI::DISABLE_UPLOADS = 1; # no need, so be safe

use lib '/home/jilles/src/hg/atheme/contrib/'; # path to Atheme.pm
use Atheme;

my $cgi = new CGI;
my ($alogin, $acookie) = ('', '');
my $atheme;

my $op = $cgi->param('op');
$op = 'login' unless defined $op;
my $me = $cgi->url(-relative=>1);

my $auth = $cgi->cookie('athemeauth');
if (defined ($auth) && $auth =~ /^(.*):(.*)$/) {
	($alogin, $acookie) = ($1, $2);
}

if ($alogin eq '' && $op ne 'logout' && ($op ne 'login' || !defined($cgi->param('login')))) {
	print $cgi->header;
	print $cgi->start_html('Atheme login');
	print $cgi->h1('Atheme login');
	print $cgi->start_form;
	print 'Login ', $cgi->textfield('login');
	print 'Password ', $cgi->password_field('password');
	$cgi->param('op', 'login');
	print $cgi->hidden(-name=>'op', -default=>'login');
	print $cgi->submit;
	print $cgi->end_form;
	print $cgi->end_html;
	exit;
}

$atheme = new Atheme(url => 'http://127.0.0.1:8080/xmlrpc'); # change if necessary

my $result;

if ($op eq 'login') {
	$result = $atheme->login({nick=>$cgi->param('login'), pass=>$cgi->param('password'), address=>$cgi->remote_host()});
	if ($result->{type} eq 'success') {
		$atheme->{nick} = $cgi->param('login');
		$atheme->{authcookie} = $result->{string};
		my $cookie = $cgi->cookie(-name=>'athemeauth',
			-value=>$atheme->{nick}.':'.$atheme->{authcookie},
			-expires=>'+1h');
		print $cgi->header(-cookie=>$cookie);
		print $cgi->start_html('Atheme login successful');
		print $cgi->h1('Atheme login successful');
		print $cgi->start_form;
		print "Click to display information about account -> ";
		$cgi->param('op', 'info');
		print $cgi->hidden(-name=>'op', -default=>'info');
		print $cgi->submit;
		print $cgi->end_form;
	} else {
		print $cgi->start_html('Atheme login failed');
		print $cgi->h1('Atheme login failed');
		print 'Fault type:', $result->{type}, $cgi->br();
		print 'Fault string:', $result->{string}, $cgi->br();
	}
	print "<p><a href=\"$me?op=logout\">Log out</a></p>\n";
	print $cgi->end_html;
	exit;
} elsif ($op eq 'logout') {
	if ($acookie ne '') {
		$result = $atheme->logout({authcookie=>$acookie, nick=>$alogin});
		my $cookie = $cgi->cookie(-name=>'athemeauth',
			-value=>'',
			-expires=>'+5m');
		print $cgi->header(-cookie=>$cookie);
	} else {
		print $cgi->header;
	}
	print $cgi->start_html('Atheme logout successful');
	print $cgi->h1('Atheme logout successful');
	print "<p><a href=\"$me?op=login\">Log in again</a></p>\n";
	print $cgi->end_html;
	exit;
}

print $cgi->header;
print $cgi->start_html('Atheme account information');
print $cgi->h1('Atheme account information');
$atheme->{svs} = 'NickServ';
$atheme->{cmd} = 'INFO';
$result = $atheme->call_svs({authcookie=>$acookie, nick=>$alogin, address=>$cgi->remote_host(), params=>$alogin});
if ($result->{type} eq 'success') {
	print 'Account information: <xmp>'.$result->{string}."</xmp>\n";
} else {
	print 'Fault type:', $result->{type}, $cgi->br();
	print 'Fault string:', $result->{string}, $cgi->br();
}
print "<p><a href=\"$me?op=logout\">Log out</a></p>\n";
print $cgi->end_html;

