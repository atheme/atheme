#!/usr/bin/env perl

# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2021 Aaron M. D. Jones <me@aaronmdjones.net>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THIS SOFTWARE IS PROVIDED "AS IS", AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#
#
#
# Takes as input (on stdin) a services database that may contain password
# hashes from the crypto/pbkdf2 module. Outputs (on stdout) the same data,
# but with those password hashes converted to the newer format used by the
# crypto/pbkdf2v2 module instead.
#
# To use this script, first shut down services. Then execute:
#
# $ mv services.db services.db.bak
# $ perl /path/to/pbkdf2-convert.pl < services.db.bak > services.db
#
# DO NOT use the same filename for input and output; that will erase your
# database instead!
#
# If this script generates any output (on stderr), do not use the results,
# remove the output file, restore your database from the backup made above,
# and file a bug report.
#
#
#
# Once you have the new database in place, you will be able to remove the
# loadmodule line for crypto/pbkdf2 from your services configuration file.
#
# You will need to add a loadmodule line for the newer crypto/pbkdf2v2
# module.
#
# If you wish to avoid the newer module rehashing all passwords as it
# successfully verifies them (due to different parameters), configure it to
# output hashes with the same parameters as the older module:
#
#     crypto {
#         pbkdf2v2_digest = "SHA512";
#         pbkdf2v2_rounds = 128000;
#         pbkdf2v2_saltlen = 16;
#     };

use strict;
use warnings;
use diagnostics;

use MIME::Base64 qw/encode_base64/;

while (readline(STDIN))
{
	chomp;

	if (m/^MU ([^ ]+) ([^ ]+) ([^ ]+) ([^ ]+) ([0-9]+) ([0-9]+) \+([^ ]+) (.+)$/)
	{
		my $entityid = $1;
		my $account = $2;
		my $pwstr = $3;
		my $email = $4;
		my $regts = $5;
		my $seents = $6;
		my $accflags = $7;
		my $rest = $8;

		if (length($pwstr) == 144 && $accflags =~ m/C/ && $pwstr =~ m/^([A-Za-z0-9]{16})([A-Fa-f0-9]{128})$/)
		{
			my $salt = $1;
			my $hash = $2;
			my $binary_hash = pack "H*", $hash;
			my $salt64 = encode_base64($salt, '');
			my $hash64 = encode_base64($binary_hash, '');

			$pwstr = sprintf '$z$26$128000$%s$%s', $salt64, $hash64;
		}

		printf 'MU %s %s %s %s %s %s +%s %s%s', $entityid, $account, $pwstr, $email, $regts, $seents, $accflags, $rest, "\n";
	}
	else
	{
		printf '%s%s', $_, "\n";
	}
}
