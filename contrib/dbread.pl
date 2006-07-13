#!/usr/bin/env perl

use strict;

open (DB, "< atheme.db")
   or die "Can not open database $!";

my (%user, %memo, %memoignore, %channel, %metadata, %serviceignore, %kline);

sub db_load {
   my ($numusers, $numchans, $numchanacs, $numklines) = (0, 0, 0, 0);
   while (<DB>) {
      # Get rid of \n
      chomp;

      # Database version
      my @item = split /\ /, $_, 2;
      die "db_load(): database version is $item[1]; I only understand version 4 (Atheme 0.2)"
         if ($item[0] eq "DBV" and $item[1] > 4);

      # Users
      if ($item[0] eq "MU") {
         my @item = split /\ /, $item[1], 9;
         # First item is username
         $user{$item[0]} = {
            password     => $item[1],
            email        => $item[2],
            registered   => $item[3],
            lastlogin    => $item[4],
            failnum      => $item[5],
            lastfailaddr => $item[6],
            lastfailtime => $item[7],
            flags        => $item[8],
         };

         $numusers++;
      }

      # Memos
      if ($item[0] eq "ME") {
         my @item = split /\ /, $item[1], 5;
         # First item is recipient
         $memo{$item[0]} = {
            sender   => $item[1],
            memotime => $item[2],
            status   => $item[3],
            text     => $item[4],
         };

         die "db_load(): memo for unknown account"
            unless $user{$item[0]};
      }

      # Memo ignores
      if ($item[0] eq "MI") {
         my @item = split /\ /, $item[1], 2;
         # First item is ignorer
         $memoignore{$item[0]} = {
            target => $item[1],
         };

         die "db_load(): invalid ignore (MI $item[0] $item[1])"
            unless $user{$item[0]} and $user{$item[1]};
      }

      # Channels
      if ($item[0] eq "MC") {
         my @item = split /\ /, $item[1], 10;
         # First argument is channel name
         $channel{$item[0]} = {
            password    => $item[1],
            founder     => $item[2], # TODO: Try linking in a user hash
            registered  => $item[3],
            used        => $item[4],
            flags       => $item[5],
            mlock_on    => $item[6],
            mlock_off   => $item[7],
            mlock_limit => $item[8],
            mlock_key   => $item[9],
         };

         $numchans++;
      }

      # Metadata
      if ($item[0] eq "MD") {
         my @item = split /\ /, $item[1], 4;
         # Arguments are {type}{name}{property} = value
         $metadata{$item[0]}{$item[1]}{$item[2]} = $item[3];
      }

      # Channel URLs
      if ($item[0] eq "UR") {
         my @item = split /\ /, $item[1], 2;
         # First argument is channel name
         $channel{$item[0]}{url} = $item[1];
      }

      # Channel entry messages
      if ($item[0] eq "EM") {
         my @item = split /\ /, $item[1], 2;
         # First argument is channel name
         $channel{$item[0]}{message} = $item[1];
      }

      # Channel access masks
      if ($item[0] eq "CA") {
         my @item = split /\ /, $item[1], 2;
         # First argument is user name
         # TODO: Disassemble chanacs and write appropriate access flags
         $user{$item[0]}{chanacs} = $item[1];

         $numchanacs++;
      }

      # Services ignores
      if ($item[0] eq "SI") {
         my @item = split /\ /, $item[1], 4;
         # First argument is mask
         $serviceignore{$item[0]} = {
            settime => $item[1],
            setby   => $item[2],
            reason  => $item[3],
         };
      }

      # K:Lines
      if ($item[0] eq "KL") {
         my @item = split /\ /, $item[1], 6;
         # First argument is user
         $kline{$item[0]} = {
            host     => $item[1],
            duration => $item[2],
            settime  => $item[3],
            setby    => $item[4],
            reason   => $item[5],
         };
         
         $numklines++;
      }

      # End
      if ($item[0] eq "DE") {
         my @item = split /\ /, $item[1], 4;
         die "db_load(): got $numusers myusers; expected $item[0]"
            unless $item[0] == $numusers;
         die "db_load(): got $numchans mychans; expected $item[1]"
            unless $item[1] == $numchans;
         die "db_load(): got $numchanacs chanacs; expected $item[2]"
            unless $item[2] == $numchanacs;
         die "db_load(): got $numklines klines; expected $item[3]"
            unless $item[3] == $numklines;
      }
   }
}

db_load();
print $user{$ARGV[0]}{password};
