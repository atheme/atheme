#!/usr/bin/perl

#use strict;

my %myusers = ( );
my %mychans = ( );

if (!$ARGV[0] || !$ARGV[1])
{
	die "usage: check_collisions.pl database1 database2";
}

# loads in a database
sub load_database
{
	my ($dbname, $id) = @_;

	open(IN, $dbname);

	while ($line = <IN>)
	{
		if ($line =~ /^MU/)
		{
			my (@tokens) = split(/ /, $line);

			print "$dbname: myuser $tokens[1] -> $tokens[3]\n";

			$myusers{$id}{$tokens[1]} = $tokens[3];
		}

		if ($line =~ /^MC/)
		{
			my (@tokens) = split(/ /, $line);

			print "$dbname: mychan $tokens[1] -> $tokens[3]\n";

			$mychans{$id}{$tokens[1]} = $tokens[3];
		}
	}

	close(IN);
}

load_database($ARGV[0], 1);
load_database($ARGV[1], 2);

print "Checking for collisions...\n";

# check for account collisions
foreach my $i (keys %{ $myusers{1} })
{
	print "$myusers{1}{$i}\n";

	if ($myusers{1}{$i} ne "" && $myusers{2}{$i} ne "")
	{
		print "Possible collision: $i\n";
	}

	if (($myusers{1}{$i} && $myusers{2}{$i}) &&
		($myusers{1}{$i} ne $myusers{2}{$i}))
	{
		print "ACCOUNT COLLISION: $i ($myusers{1}{$i} != $myusers{2}{$i})\n";
	}
}

# check for channel collisions
foreach my $i (keys %{ $mychans{1} })
{
	if (($mychans{1}{$i} && $mychans{2}{$i}) &&
		($mychans{1}{$i} ne $mychans{2}{$i}))
	{
		print "CHANNEL COLLISION: $i ($mychans{1}{$i} != $mychans{2}{$i})\n";
	}
}

print "Done.\n";
