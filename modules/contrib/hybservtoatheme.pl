#!/usr/bin/perl -w
# 
#  Copyright (c) 2005-2008, Jilles Tjoelker
#  All rights reserved.
# 
#  Redistribution and use in source and binary forms, with
#  or without modification, are permitted provided that the
#  following conditions are met:
# 
#  1. Redistributions of source code must retain the above
#     copyright notice, this list of conditions and the
#     following disclaimer.
#  2. Redistributions in binary form must reproduce the
#     above copyright notice, this list of conditions and
#     the following disclaimer in the documentation and/or
#     other materials provided with the distribution.
# 
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
#  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
#  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
#  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
#  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
#  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY
#  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
#  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
#  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
#  OF SUCH DAMAGE.
#
# $Id: hybservtoatheme.pl 8315 2007-05-24 16:45:39Z jilles $
#
# HybServ2/dancer-services to Atheme database converter
# - Reads nick.db and chan.db from the current directory.
# - Writes an Atheme flatfile db to stdout.
# - Certain errors abort the script and give a message on stderr.
# - Several other problematic cases are noted in the output as comments.
# - Derived from my hybserv to ratbox-services converter.
#
# Some problems/caveats:
# - Flag and level constants are hard-coded, see
#   hybserv/include/{nick,chan}serv.h.
# - Access levels cannot be fully converted, but the code here could be
#   improved a bit.
# - Master nicks are converted to accounts; any linked nicks are converted
#   to nicks attached to that account. This means users should use SET MASTER
#   an appropriate nick in HybServ2; this cannot be easily changed later.
# - Available flags are different; there is information loss here.
# - The script is probably easy to adapt to other HybServ2 derivatives,
#   please send me the patch if you do.
#
# Jilles Tjoelker
# <jilles@stack.nl>
# http://www.stack.nl/~jilles/irc/

print "# hybservtoatheme.pl Copyright 2005-2008 Jilles Tjoelker\n";
print "DBV 5\n";
print "CF +vVhHoOtsriRfAb\n";

$fakets = time() || 2147483647;
$istheia = 0;

print "# Converting nick.db\n";
# first pass to take care of emails on non-master nicks
open(NICKDB, "<nick.db");
$email = '';
while (<NICKDB>) {
	if (/^->EMAIL (.*)$/) {
		$email = $1;
		$email =~ s/^<//;
		$email =~ s/>$//;
		if ($email ne '' && !defined($anyemail{$nick})) {
			$anyemail{$nick} = $email;
		}
	} elsif (/^->LINK (.*)$/) {
		$master = $1;
		if ($email ne '') {
			if (!defined($anyemail{$master})) {
				print "# Taking email for $master from $nick\n";
				$anyemail{$master} = $email;
			} elsif ($anyemail{$master} ne $email) {
				print "# Ignoring extra email for $master from $nick\n";
			}
		}
	} elsif (/^->/) {
	} elsif (/^([^ ]+) ([0-9]+) ([0-9]+) ([0-9]+)$/) {
		$nick = $1;
		$email = '';
	}
}
close(NICKDB);

open(NICKDB, "<nick.db");
$nick = '';
$password = '';
$email = '';
$cloak = '';
@access = ();
while (<NICKDB>) {
	if (/^->([a-zA-Z]+) (.*)$/) {
		$word = $1;
		$args = $2;
		if ($word eq 'PASS') {
			$password = $args;
		} elsif ($word eq 'LINK') {
			# Store master nick, might be wrong in channel
			# access list :(
			$masternick{$nick} = $args;
			print "MN $args $nick $regtime $lastseentime\n";
			$nick = '';
		} elsif ($word eq 'LASTUH') {
			$lastuh = $args;
			$lastuh =~ s/ /@/;
		} elsif ($word eq 'CLOAK') {
			$cloak = $args;
		} elsif ($word eq 'HOST') {
			push @access, $args;
		}
	} elsif (/^([^ ]+) ([0-9]+) ([0-9]+) ([0-9]+)$/) {
		if ($nick ne '') {
			print "MU $nick $password $email $regtime $lastseentime 0 0 0 $athflags\n";
			print "MD U $nick private:host:vhost $lastuh\n" if ($lastuh ne '');
			if ($cloak ne '') {
				if ($hsflags & 0x00400000) {
					print "MD U $nick private:usercloak $cloak\n";
				} else {
					# disabled cloaks are used as marks
					print "MD U $nick private:mark:setter unknown\n";
					print "MD U $nick private:mark:reason $cloak\n";
				}
			}
			print "MD U $nick private:extendchans 1\n" if ($hsflags & 0x01000000) && $istheia;
			print "MD U $nick private:unfiltered 1\n" if ($hsflags & 0x00800000) && $istheia;
			print "AC $nick $_\n" for @access;
			print "MN $nick $nick $regtime $lastseentime\n";
			$masternick{$nick} = $nick;
		}
		$nick = $1;
		$hsflags = $2;
		$regtime = $3;
		$lastseentime = $4;
		$password = '*';
		if (defined($anyemail{$nick})) {
			$email = $anyemail{$nick};
		} else {
			$email = 'noemail';
		}
		$lastuh = '';
		$cloak = '';
		@access = ();
		$athflags = 0;
		#$athflags |= ? if ($hsflags & 16); # private
		$athflags |= 1 if ($hsflags & 4); # operator/hold
		$athflags |= 0x10 if ($hsflags & 0x8000); # hideemail/hidemail
		$athflags |= 0x40 if !($hsflags & 0x2000); # !memos/nomemo
		$athflags |= 0x100; # cryptpass
		$athflags |= 0x2010 if $hsflags & 0x4000; # hideall/private+hidemail
		$athflags |= 0x2000 if $hsflags & 0x40000; # hideaddr/private
		$athflags |= 0x2010 if $hsflags & 0x10; # private/private+hidemail
		# transform either of noregister and nochanops to suspend
		# hope atheme doesn't mind we don't know the suspender...
		#$athflags |= 1 if ($hsflags & 0x300000);
		# theia:
		# 0x00800000 unfiltered
		# 0x01000000 extendchans
		# many other flags unused, see hybserv/include/nickserv.h
		if ($hsflags & 0x80) {
			# "forbidden" nick, hybserv will kill
			# sort of a friendlier network-wide nick resv
			print "# Ignoring forbidden nick $nick\n";
			$nick = '';
		}
	} elsif (/^;( ?)(.*)$/) {
		print "# $2\n";
		$istheia = 1 if $2 =~ /^(theia|dancer)/i;
		print "# theia/dancer detected\n" if $istheia;
	} else {
		print STDERR "Unrecognized line $. in nick.db:\n";
		print STDERR $_;
		exit(1);
	}
}
if ($nick ne '') {
	print "MU $nick $password $email $regtime $lastseentime 0 0 0 $athflags\n";
	print "MD U $nick private:host:vhost $lastuh\n" if ($lastuh ne '');
	if ($cloak ne '') {
		if ($hsflags & 0x00400000) {
			print "MD U $nick private:usercloak $cloak\n";
		} else {
			# disabled cloaks are used as marks
			print "MD U $nick private:mark:setter unknown\n";
			print "MD U $nick private:mark:reason $cloak\n";
		}
	}
	print "MD U $nick private:extendchans 1\n" if ($hsflags & 0x01000000) && $istheia;
	print "MD U $nick private:unfiltered 1\n" if ($hsflags & 0x00800000) && $istheia;
	print "AC $nick $_\n" for @access;
	print "MN $nick $nick $regtime $lastseentime\n";
	$masternick{$nick} = $nick;
}
close(NICKDB);

print "# Converting chan.db\n";
open(CHANDB, "<chan.db");
$missingfounderchanacs = '';
while (<CHANDB>) {
	if (/^->([a-zA-Z]+) (.*)$/) {
		$word = $1;
		$args = $2;
		if ($word eq 'FNDR') {
			$founder = $args;
			# strip off dancer-services last seen time
			$founder =~ s/ .*//;
		} elsif ($word eq 'SUCCESSOR') {
			$successor = $args;
			# strip off dancer-services last seen time
			$successor =~ s/ .*//;
		} elsif ($word eq 'PASS') {
			$password = $args;
		} elsif ($word eq 'TOPIC') {
			$topic = $args;
			$topic =~ s/^://;
		} elsif ($word eq 'LIMIT') {
			$lockedlimit = $args;
		} elsif ($word eq 'KEY') {
			$lockedkey = $args;
		} elsif ($word eq 'FORWARD') {
			$lockedext .= " f$args" if $args =~ /^#\S*$/;
			$lockedext =~ s/^ //;
		} elsif ($word eq 'THROTTLE') {
			$lockedext .= " J$args" if $args =~ /^\d+,\d+$/;
			$lockedext =~ s/^ //;
		} elsif ($word eq 'DLINE') {
			#$lockedext .= " D$args" if $args =~ /^\d+,\d+$/;
			#$lockedext =~ s/^ //;
		} elsif ($word eq 'MON') {
			#$lockedon |= 0x4 if $args & 0x10; # l
			#$lockedon |= 0x2 if $args & 0x20; # k
			$lockedon |= 0x80 if $args & 0x40; # s
			$lockedon |= 0x40 if $args & 0x80; # p
			$lockedon |= 0x10 if $args & 0x100; # n
			$lockedon |= 0x100 if $args & 0x200; # t
			$lockedon |= 0x8 if $args & 0x400; # m
			$lockedon |= 0x1 if $args & 0x800; # i
			if ($istheia) {
				$lockedon |= 0x1000 if $args & 0x1000; # c
				$lockedon |= 0x8000 if $args & 0x4000; # g
				$lockedon |= 0x20000 if $args & 0x20000; # P
				$lockedon |= 0x80000 if $args & 0x40000; # Q
				$lockedon |= 0x2000 if $args & 0x80000; # r
				$lockedon |= 0x4000 if $args & 0x100000; # z
				$lockedon |= 0x10000 if $args & 0x400000; # L
				$lockedon |= 0x200000 if $args & 0x800000; # R
			}
			# anonops, hybrid7.0 only and often disabled
			#$lockedon |= 'a' if $args & 0x4000; # a
		} elsif ($word eq 'MOFF') {
			$lockedoff |= 0x4 if $args & 0x10; # l
			$lockedoff |= 0x2 if $args & 0x20; # k
			$lockedoff |= 0x80 if $args & 0x40; # s
			$lockedoff |= 0x40 if $args & 0x80; # p
			$lockedoff |= 0x10 if $args & 0x100; # n
			$lockedoff |= 0x100 if $args & 0x200; # t
			$lockedoff |= 0x8 if $args & 0x400; # m
			$lockedoff |= 0x1 if $args & 0x800; # i
			if ($istheia) {
				$lockedoff |= 0x1000 if $args & 0x1000; # c
				$lockedoff |= 0x8000 if $args & 0x4000; # g
				$lockedoff |= 0x20000 if $args & 0x20000; # P
				$lockedoff |= 0x80000 if $args & 0x40000; # Q
				$lockedoff |= 0x2000 if $args & 0x80000; # r
				$lockedoff |= 0x4000 if $args & 0x100000; # z
				$lockedoff |= 0x10000 if $args & 0x400000; # L
				$lockedoff |= 0x200000 if $args & 0x800000; # R
				$lockedext .= ' f' if $args & 0x2000; # f
				$lockedext .= ' J' if $args & 0x10000; # J
				#$lockedext .= ' D' if $args & 0x200000; # D
				$lockedext =~ s/^ //;
			}
			# anonops, hybrid7.0 only and often disabled
			#$lockedoff |= 'a' if $args & 0x4000; # a
		} elsif ($word eq 'ENTRYMSG') {
			$entrymsg = $args;
			$entrymsg =~ s/^://;
		} elsif ($word eq 'URL') {
			$url = $args;
		} elsif ($word eq 'EMAIL') {
			$email = $args;
		} elsif ($word eq 'ALVL') {
			# ->ALVL always appears, and we have all required
			# data for the channels table now
			print "MC $chan 0 $founder $regtime $lastusedtime $athflags $lockedon $lockedoff $lockedlimit $lockedkey\n";
			if ($topic ne '') {
				print "MD C $chan private:topic:text $topic\n";
				print "MD C $chan private:topic:setter unknown\n";
				print "MD C $chan private:topic:ts $fakets\n";
			}
			print "MD C $chan private:entrymsg $entrymsg\n" if ($entrymsg ne '');
			print "MD C $chan url $url\n" if ($url ne '');
			print "MD C $chan email $email\n" if ($email ne '');
			print "MD C $chan private:mlockext $lockedext\n" if ($lockedext ne '');
			if ($args =~ /^(-?[0-9]+) (-?[0-9]+) (-?[0-9]+) (-?[0-9]+) (-?[0-9]+) (-?[0-9]+) (-?[0-9]+) (-?[0-9]+) (-?[0-9]+) (-?[0-9]+) (-?[0-9]+) (-?[0-9]+) (-?[0-9]+) (-?[0-9]+) (-?[0-9]+)$/) { # w/ halfops
				$nooplvl = $1;
				$autovoicelvl = $2;
				$voicelvl = $3;
				$accessmodlvl = $4;
				$invitelvl = $5;
				$autohalfoplvl = $6;
				$halfoplvl = $7;
				$autooplvl = $8;
				$oplvl = $9;
				$unbanselflvl = $10;
				$akicklvl = $11;
				$clearlvl = $12;
				$setlvl = $13;
				$superoplvl = $14;
				$founderlvl = $15;
				$topiclvl = $halfoplvl;
			} elsif ($args =~ /^(-?[0-9]+) (-?[0-9]+) (-?[0-9]+) (-?[0-9]+) (-?[0-9]+) (-?[0-9]+) (-?[0-9]+) (-?[0-9]+) (-?[0-9]+) (-?[0-9]+) (-?[0-9]+) (-?[0-9]+) (-?[0-9]+)$/) { # no halfops
				$nooplvl = $1;
				$autovoicelvl = $2;
				$voicelvl = $3;
				$accessmodlvl = $4;
				$invitelvl = $5;
				$autooplvl = $6;
				$oplvl = $7;
				$unbanselflvl = $8;
				$akicklvl = $9;
				$clearlvl = $10;
				$setlvl = $11;
				$superoplvl = $12;
				$founderlvl = $13;
				$autohalfoplvl = $autooplvl;
				$halfoplvl = $oplvl;
				$topiclvl = $oplvl;
			} else {
				print STDERR "Unrecognized access level line $. in chan.db channel $chan\n";
				exit(1);
			}
			if ($autovoicelvl == 0) {
				print "CA $chan *!*@* +V\n";
			}
		} elsif ($word eq 'ACCESS') {
			$args =~ /^([^ ]+) (-?[0-9]+)/;
			$nick = $1;
			$hslevel = $2;
			$athlevel = '';
			$athlevel .= 'A' if ($hslevel >= 1);
			$athlevel .= 'i' if ($hslevel >= $invitelvl);
			$athlevel .= 'v' if ($hslevel >= $voicelvl);
			# Hmm AKICK vs ACCESS?
			$athlevel .= 'r' if ($hslevel >= $akicklvl);
			$athlevel .= 'h' if ($hslevel >= $halfoplvl);
			$athlevel .= 't' if ($hslevel >= $topiclvl);
			$athlevel .= 'o' if ($hslevel >= $oplvl);
			$athlevel .= 'R' if ($hslevel >= $clearlvl);
			$athlevel .= 'f' if ($hslevel >= $accessmodlvl && ($istheia || $hslevel >= $clearlvl));
			$athlevel .= 's' if ($hslevel >= $setlvl);
			$athlevel = 'AivrhtoRfs' if ($nick eq $founder);
			$athlevel .= 'V' if ($hslevel >= $autovoicelvl);
			$athlevel .= 'H' if ($hslevel >= $autohalfoplvl);
			$athlevel .= 'O' if ($hslevel >= $autooplvl);
			#$athlevel .= '?' if ($hslevel >= $superoplvl);
			#$athlevel ?? if ($nick eq $successor);
			if ($athlevel eq '') {
				print "# Skipping access entry $chan $nick @ level zero\n";
			} elsif ($nick =~ /@/) {
				print "CA $chan $nick +$athlevel\n";
			} else {
				$master = $masternick{$nick};
				if (defined($master)) {
					print "CA $chan $master +$athlevel\n";
					$missingfounderchanacs = '' if $master eq $founder;
				} else {
					print "# Skipping access entry $chan $nick @ level $athlevel because of invalid nick\n";
				}
			}
		} elsif ($word eq 'AKICK') {
			$args =~ /^([^ ]+) :(.*)$/;
			$banmask = $1;
			$reason = $2;
			print "CA $chan $banmask +b\n";
			print "MD A $chan:$banmask reason $reason\n";
		}
	} elsif (/^(\#[^ ]*) ([0-9]+) ([0-9]+) ([0-9]+)$/) {
		print "CA $missingfounderchanacs $founder +AivrhtoRfs\n" if $missingfounderchanacs ne '';
		$chan = $1;
		$hsflags = $2;
		$regtime = $3;
		$lastusedtime = $4;
		$password = '';
		$topic = '';
		$successor = '';
		$lockedlimit = 0;
		$lockedkey = '';
		$lockedon = 0;
		$lockedoff = 0;
		$lockedext = '';
		$entrymsg = '';
		$url = '';
		$email = '';
		$athflags = 0;
		$athflags |= 1 if ($hsflags & 0x200); # noexpire/hold
		$athflags |= 8 if ($hsflags & 8); # secureops/secure
		$athflags |= 16 if ($hsflags & 0x1000); # verbose
		$athflags |= 320 if ($hsflags & 2); # topiclock/topiclock+keeptopic
		$athflags |= 512 if ($hsflags & 0x400); # guard
		$athflags |= 1024 if ($hsflags & 0x1); # private
		$missingfounderchanacs = $chan;
	} elsif (/^;( ?)(.*)$/) {
		print "# $2\n";
	} else {
		print STDERR "Unrecognized line $. in chan.db:\n";
		print STDERR $_;
		exit(1);
	}
}
print "CA $missingfounderchanacs $founder +AivrhtoRfs\n" if $missingfounderchanacs ne '';
close(CHANDB);
print "# End of hybservtoatheme.pl output\n";
