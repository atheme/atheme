#!/usr/bin/env perl

# SPDX-License-Identifier: ISC
# SPDX-URL: https://spdx.org/licenses/ISC.html
#
# Copyright (C) 2022 Aaron M. D. Jones <me@aaronmdjones.net>
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
# Takes as input (on stdin) a services database. Outputs (on stdout) the same
# database, but with user accounts also having the BADPASSWDMSG flag enabled.
#
# To use this script, first shut down services. Then execute:
#
# $ mv services.db services.db.bak
# $ perl /path/to/add_badpasswdmsg.pl < services.db.bak > services.db
#
# DO NOT use the same filename for input and output; that will erase your
# database instead!
#
# If this script generates any output (on stderr), do not use the results,
# remove the output file, restore your database from the backup made above,
# and file a bug report.

use strict;
use warnings;
use diagnostics;

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

		if ($accflags !~ m/B/)
		{
			$accflags = join '', $accflags, 'B';
		}

		printf 'MU %s %s %s %s %s %s +%s %s%s', $entityid, $account, $pwstr, $email, $regts, $seents, $accflags, $rest, "\n";
	}
	else
	{
		printf '%s%s', $_, "\n";
	}
}
