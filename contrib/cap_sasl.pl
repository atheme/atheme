use strict;
use Irssi;
use vars qw($VERSION %IRSSI);
# $Id$

use MIME::Base64;

use constant CHALLENGE_SIZE => 32;

$VERSION = "1.7";

%IRSSI = (
    authors     => 'Michael Tharp and Jilles Tjoelker',
    contact     => 'gxti@partiallystapled.com',
    name        => 'cap_sasl.pl',
    description => 'Implements SASL authentication and enables CAP "multi-prefix"',
    license     => 'GNU General Public License',
    url         => 'http://ircv3.atheme.org/extensions/sasl-3.1',
);

my %sasl_auth = ();
my %mech = ();

sub timeout;

sub server_connected {
	my $server = shift;
	if (uc $server->{chat_type} eq 'IRC') {
		$server->send_raw_now("CAP LS");
	}
}

sub event_cap {
	my ($server, $args, $nick, $address) = @_;
	my ($subcmd, $caps, $tosend, $sasl);

	$tosend = '';
	$sasl = $sasl_auth{$server->{tag}};
	if ($args =~ /^\S+ (\S+) :(.*)$/) {
		$subcmd = uc $1;
		$caps = ' '.$2.' ';
		if ($subcmd eq 'LS') {
			$tosend .= ' multi-prefix' if $caps =~ / multi-prefix /i;
			$tosend .= ' sasl' if $caps =~ / sasl /i && defined($sasl);
			$tosend =~ s/^ //;
			$server->print('', "CLICAP: supported by server:$caps");
			if (!$server->{connected}) {
				if ($tosend eq '') {
					$server->send_raw_now("CAP END");
				} else {
					$server->print('', "CLICAP: requesting: $tosend");
					$server->send_raw_now("CAP REQ :$tosend");
				}
			}
			Irssi::signal_stop();
		} elsif ($subcmd eq 'ACK') {
			$server->print('', "CLICAP: now enabled:$caps");
			if ($caps =~ / sasl /i) {
				$sasl->{buffer} = '';
				$sasl->{step} = 0;
				if ($mech{$sasl->{mech}}) {
					$server->send_raw_now("AUTHENTICATE " . $sasl->{mech});
					Irssi::timeout_add_once(7500, \&timeout, $server->{tag});
				} else {
					$server->print('', 'SASL: attempted to start unknown mechanism "' . $sasl->{mech} . '"');
				}
			}
			elsif (!$server->{connected}) {
				$server->send_raw_now("CAP END");
			}
			Irssi::signal_stop();
		} elsif ($subcmd eq 'NAK') {
			$server->print('', "CLICAP: refused:$caps");
			if (!$server->{connected}) {
				$server->send_raw_now("CAP END");
			}
			Irssi::signal_stop();
		} elsif ($subcmd eq 'LIST') {
			$server->print('', "CLICAP: currently enabled:$caps");
			Irssi::signal_stop();
		}
	}
}

sub event_authenticate {
	my ($server, $args, $nick, $address) = @_;
	my $sasl = $sasl_auth{$server->{tag}};
	return unless $sasl && $mech{$sasl->{mech}};

	$sasl->{buffer} .= $args;
	return if length($args) == 400;

	my $data = ($sasl->{buffer} eq '+') ? '' : decode_base64($sasl->{buffer});
	my $out = $mech{$sasl->{mech}}($sasl, $data);

	if (defined $out) {
		$out = ($out eq '') ? '+' : encode_base64($out, '');

		while (length $out >= 400) {
			my $subout = substr($out, 0, 400, '');
			$server->send_raw_now("AUTHENTICATE $subout");
		}
		if (length $out) {
			$server->send_raw_now("AUTHENTICATE $out");
		} else {
			# Last piece was exactly 400 bytes, we have to send
			# some padding to indicate we're done.
			$server->send_raw_now("AUTHENTICATE +");
		}
	} else {
		$server->send_raw_now("AUTHENTICATE *");
	}

	$sasl->{buffer} = "";
	Irssi::signal_stop();
}

sub event_saslend {
	my ($server, $args, $nick, $address) = @_;

	my $data = $args;
	$data =~ s/^\S+ :?//;
	# need this to see it, ?? -- jilles
	$server->print('', $data);
	if (!$server->{connected}) {
		$server->send_raw_now("CAP END");
	}
}

sub timeout {
	my $tag = shift;
	my $server = Irssi::server_find_tag($tag);
	if ($server && !$server->{connected}) {
		$server->print('', "SASL: authentication timed out", MSGLEVEL_CLIENTERROR);
		$server->send_raw_now("CAP END");
	}
}

sub cmd_sasl {
	my ($data, $server, $item) = @_;

	if ($data ne '') {
		Irssi::command_runsub ('sasl', $data, $server, $item);
	} else {
		cmd_sasl_show(@_);
	}
}

sub cmd_sasl_set {
	my ($data, $server, $item) = @_;

	if (my($net, $u, $p, $m) = $data =~ /^(\S+) (\S+) (\S+) (\S+)$/) {
		if ($mech{uc $m}) {
			$sasl_auth{$net}{user} = $u;
			$sasl_auth{$net}{password} = $p;
			$sasl_auth{$net}{mech} = uc $m;
			Irssi::print("SASL: added $net: [$m] $sasl_auth{$net}{user} *");
		} else {
			Irssi::print("SASL: unknown mechanism $m", MSGLEVEL_CLIENTERROR);
		}
	} elsif ($data =~ /^(\S+)$/) {
		$net = $1;
		if (defined($sasl_auth{$net})) {
			delete $sasl_auth{$net};
			Irssi::print("SASL: deleted $net");
		} else {
			Irssi::print("SASL: no entry for $net");
		}
	} else {
		Irssi::print("SASL: usage: /sasl set <net> <user> <password or keyfile> <mechanism>");
	}
}

sub cmd_sasl_show {
	#my ($data, $server, $item) = @_;
	my $net;
	my $count = 0;

	foreach $net (keys %sasl_auth) {
		Irssi::print("SASL: $net: [$sasl_auth{$net}{mech}] $sasl_auth{$net}{user} *");
		$count++;
	}
	Irssi::print("SASL: no networks defined") if !$count;
}

sub cmd_sasl_save {
	#my ($data, $server, $item) = @_;
	my $file = Irssi::get_irssi_dir()."/sasl.auth";
	open FILE, "> $file" or return;
	chmod(0600, $file);
	foreach my $net (keys %sasl_auth) {
		printf FILE ("%s\t%s\t%s\t%s\n", $net, $sasl_auth{$net}{user}, $sasl_auth{$net}{password}, $sasl_auth{$net}{mech});
	}
	close FILE;
	Irssi::print("SASL: auth saved to $file");
}

sub cmd_sasl_load {
	#my ($data, $server, $item) = @_;
	my $file = Irssi::get_irssi_dir()."/sasl.auth";

	open FILE, "< $file" or return;
	%sasl_auth = ();
	while (<FILE>) {
		chomp;
		my ($net, $u, $p, $m) = split (/\t/, $_, 4);
		$m ||= "PLAIN";
		if ($mech{uc $m}) {
			$sasl_auth{$net}{user} = $u;
			$sasl_auth{$net}{password} = $p;
			$sasl_auth{$net}{mech} = uc $m;
		} else {
			Irssi::print("SASL: unknown mechanism $m", MSGLEVEL_CLIENTERROR);
		}
	}
	close FILE;
	Irssi::print("SASL: auth loaded from $file");
}

sub cmd_sasl_mechanisms {
	Irssi::print("SASL: mechanisms supported: " . join(", ", sort keys %mech));
}

Irssi::signal_add_first('server connected', \&server_connected);
Irssi::signal_add('event cap', \&event_cap);
Irssi::signal_add('event authenticate', \&event_authenticate);
Irssi::signal_add('event 903', 'event_saslend');
Irssi::signal_add('event 904', 'event_saslend');
Irssi::signal_add('event 905', 'event_saslend');
Irssi::signal_add('event 906', 'event_saslend');
Irssi::signal_add('event 907', 'event_saslend');

Irssi::command_bind('sasl', \&cmd_sasl);
Irssi::command_bind('sasl load', \&cmd_sasl_load);
Irssi::command_bind('sasl save', \&cmd_sasl_save);
Irssi::command_bind('sasl set', \&cmd_sasl_set);
Irssi::command_bind('sasl show', \&cmd_sasl_show);
Irssi::command_bind('sasl mechanisms', \&cmd_sasl_mechanisms);

$mech{PLAIN} = sub {
	my($sasl, $data) = @_;
	my $u = $sasl->{user};
	my $p = $sasl->{password};
	return join("\0", $u, $u, $p);
};

$mech{EXTERNAL} = sub {
	my($sasl, $data) = @_;
	return $sasl->{user} // "";
};

if (eval {require Crypt::PK::ECC}) {
	$mech{'ECDSA-NIST256P-CHALLENGE'} = sub {
		my($sasl, $data) = @_;
		my $u = $sasl->{user};
		my $k = $sasl->{password};
		my $step = ++$sasl->{step};
		if (!-f $k) {
			Irssi::print("SASL: key file '$k' not found", MSGLEVEL_CLIENTERROR);
			return;
		}
		my $pk = eval {Crypt::PK::ECC->new($k)};
		if ($@ || !$pk || !$pk->is_private) {
			Irssi::print("SASL: no private key in file '$k'", MSGLEVEL_CLIENTERROR);
			return;
		}
		if ($step == 1) {
			if (length $data == CHALLENGE_SIZE) {
				my $sig = $pk->sign_hash($data);
				return $u."\0".$u."\0".$sig;
			} elsif (length $data) {
				return;
			} else {
				return $u."\0".$u;
			}
		}
		elsif ($step == 2) {
			if (length $data == CHALLENGE_SIZE) {
				return $pk->sign_hash($data);
			} else {
				return;
			}
		}
	};
};

cmd_sasl_load();

# vim: ts=4:sw=4
