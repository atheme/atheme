package Atheme::MemoServ;

use strict;
use warnings;
use utf8;

use base qw/Atheme/;

=head1 NAME

Atheme::MemoServ - Perl interface to Atheme's XML-RPC methods

=head1 DESCRIPTION

This class provides an interface to the XML-RPC methods of the Atheme IRC Services.

=head1 METHODS

These are all either virtual or helper methods. They are being implemented in service-specific classes.

=head2 new

Services constructor. Takes a hash as argument:
   my $svs = new Atheme::MemoServ(url => "http://localhost:8000");

=cut

sub new {
   my ($self, %arg) = @_;

   # Call base constructor
   my $svs = $self->SUPER::new(%arg);

   $svs->{svs} = "MemoServ";
   $svs
}

=head2 delete

DELETE allows you to delete memos from your
inbox. You can either delete all memos with
the all parameter, or specify a memo number.
You can obtain a memo number by using the LIST
command.
 
You can also SEND, READ, LIST and FORWARD memos.
 
Syntax: DELETE all|<memo number>

=cut

sub delete {
   my ($self, $args) = @_;

   $self->{cmd} = "delete";

   # Change the default text for the dispatch table
   $self->{fault_strings}->{fault_needmoreparams} = "Insufficient parameters.";
   $self->{fault_strings}->{fault_badparams}      = "Invalid parameters.";
   $self->{fault_strings}->{fault_nosuch_source}  = "No such source.";
   $self->{fault_strings}->{fault_nosuch_target}  = "No such target.";
   $self->{fault_strings}->{fault_authfail}       = "Authentication failed.";
   $self->{fault_strings}->{fault_noprivs}        = "Insufficient privileges.";
   $self->{fault_strings}->{fault_nosuch_key}     = "No such key.";
   $self->{fault_strings}->{fault_alreadyexists}  = "Item already exists.";
   $self->{fault_strings}->{fault_toomany}        = "Too many items.";
   $self->{fault_strings}->{fault_emailfail}      = "Email verification failed.";
   $self->{fault_strings}->{fault_notverified}    = "Action not verified.";
   $self->{fault_strings}->{fault_nochange}       = "No change.";
   $self->{fault_strings}->{fault_already_authed} = "You are already authenticated.";
   $self->{fault_strings}->{fault_unimplemented}  = "Method not implemented.";

   $self->call_svs ($args)
}

=head2 forward

FORWARD allows you to forward a memo to another
account. Useful for a variety of reasons.
 
You can also SEND, DELETE, LIST or READ memos.
 
Syntax: FORWARD <user|nick> <memo number>

=cut

sub forward {
   my ($self, $args) = @_;

   $self->{cmd} = "forward";

   # Change the default text for the dispatch table
   $self->{fault_strings}->{fault_needmoreparams} = "Insufficient parameters.";
   $self->{fault_strings}->{fault_badparams}      = "Invalid parameters.";
   $self->{fault_strings}->{fault_nosuch_source}  = "No such source.";
   $self->{fault_strings}->{fault_nosuch_target}  = "No such target.";
   $self->{fault_strings}->{fault_authfail}       = "Authentication failed.";
   $self->{fault_strings}->{fault_noprivs}        = "Insufficient privileges.";
   $self->{fault_strings}->{fault_nosuch_key}     = "No such key.";
   $self->{fault_strings}->{fault_alreadyexists}  = "Item already exists.";
   $self->{fault_strings}->{fault_toomany}        = "Too many items.";
   $self->{fault_strings}->{fault_emailfail}      = "Email verification failed.";
   $self->{fault_strings}->{fault_notverified}    = "Action not verified.";
   $self->{fault_strings}->{fault_nochange}       = "No change.";
   $self->{fault_strings}->{fault_already_authed} = "You are already authenticated.";
   $self->{fault_strings}->{fault_unimplemented}  = "Method not implemented.";

   $self->call_svs ($args)
}

=head2 help

HELP displays help information on all
commands in services.
 
Syntax: HELP <command> [parameters]

=cut

sub help {
   my ($self, $args) = @_;

   $self->{cmd} = "help";

   # Change the default text for the dispatch table
   $self->{fault_strings}->{fault_needmoreparams} = "Insufficient parameters.";
   $self->{fault_strings}->{fault_badparams}      = "Invalid parameters.";
   $self->{fault_strings}->{fault_nosuch_source}  = "No such source.";
   $self->{fault_strings}->{fault_nosuch_target}  = "No such target.";
   $self->{fault_strings}->{fault_authfail}       = "Authentication failed.";
   $self->{fault_strings}->{fault_noprivs}        = "Insufficient privileges.";
   $self->{fault_strings}->{fault_nosuch_key}     = "No such key.";
   $self->{fault_strings}->{fault_alreadyexists}  = "Item already exists.";
   $self->{fault_strings}->{fault_toomany}        = "Too many items.";
   $self->{fault_strings}->{fault_emailfail}      = "Email verification failed.";
   $self->{fault_strings}->{fault_notverified}    = "Action not verified.";
   $self->{fault_strings}->{fault_nochange}       = "No change.";
   $self->{fault_strings}->{fault_already_authed} = "You are already authenticated.";
   $self->{fault_strings}->{fault_unimplemented}  = "Method not implemented.";

   $self->call_svs ($args)
}

=head2 ignore

IGNORE allows you to ignore memos from another
user. Possible reasons include inbox spamming,
annoying users, bots that have figured out how
to register etc.
 
You can add up to 40 users to your ignore list
 
Syntax: IGNORE ADD|DEL|LIST|CLEAR <account>

=cut

sub ignore {
   my ($self, $args) = @_;

   $self->{cmd} = "ignore";

   # Change the default text for the dispatch table
   $self->{fault_strings}->{fault_needmoreparams} = "Insufficient parameters.";
   $self->{fault_strings}->{fault_badparams}      = "Invalid parameters.";
   $self->{fault_strings}->{fault_nosuch_source}  = "No such source.";
   $self->{fault_strings}->{fault_nosuch_target}  = "No such target.";
   $self->{fault_strings}->{fault_authfail}       = "Authentication failed.";
   $self->{fault_strings}->{fault_noprivs}        = "Insufficient privileges.";
   $self->{fault_strings}->{fault_nosuch_key}     = "No such key.";
   $self->{fault_strings}->{fault_alreadyexists}  = "Item already exists.";
   $self->{fault_strings}->{fault_toomany}        = "Too many items.";
   $self->{fault_strings}->{fault_emailfail}      = "Email verification failed.";
   $self->{fault_strings}->{fault_notverified}    = "Action not verified.";
   $self->{fault_strings}->{fault_nochange}       = "No change.";
   $self->{fault_strings}->{fault_already_authed} = "You are already authenticated.";
   $self->{fault_strings}->{fault_unimplemented}  = "Method not implemented.";

   $self->call_svs ($args)
}

=head2 list

LIST shows you your memos in your inbox,
including who sent them and when. To read a
memo, use the READ command. You can also
DELETE or FORWARD a memo.
 
Syntax: LIST

=cut

sub list {
   my ($self, $args) = @_;

   $self->{cmd} = "list";

   # Change the default text for the dispatch table
   $self->{fault_strings}->{fault_needmoreparams} = "Insufficient parameters.";
   $self->{fault_strings}->{fault_badparams}      = "Invalid parameters.";
   $self->{fault_strings}->{fault_nosuch_source}  = "No such source.";
   $self->{fault_strings}->{fault_nosuch_target}  = "No such target.";
   $self->{fault_strings}->{fault_authfail}       = "Authentication failed.";
   $self->{fault_strings}->{fault_noprivs}        = "Insufficient privileges.";
   $self->{fault_strings}->{fault_nosuch_key}     = "No such key.";
   $self->{fault_strings}->{fault_alreadyexists}  = "Item already exists.";
   $self->{fault_strings}->{fault_toomany}        = "Too many items.";
   $self->{fault_strings}->{fault_emailfail}      = "Email verification failed.";
   $self->{fault_strings}->{fault_notverified}    = "Action not verified.";
   $self->{fault_strings}->{fault_nochange}       = "No change.";
   $self->{fault_strings}->{fault_already_authed} = "You are already authenticated.";
   $self->{fault_strings}->{fault_unimplemented}  = "Method not implemented.";

   $self->call_svs ($args)
}

=head2 read

READ allows you to read a memo that another
user has sent you. You can obtain a list of
memos from the LIST command, or delete a 
memo with the DELETE command.
 
Syntax: READ <memo number>

=cut

sub read {
   my ($self, $args) = @_;

   $self->{cmd} = "read";

   # Change the default text for the dispatch table
   $self->{fault_strings}->{fault_needmoreparams} = "Insufficient parameters.";
   $self->{fault_strings}->{fault_badparams}      = "Invalid parameters.";
   $self->{fault_strings}->{fault_nosuch_source}  = "No such source.";
   $self->{fault_strings}->{fault_nosuch_target}  = "No such target.";
   $self->{fault_strings}->{fault_authfail}       = "Authentication failed.";
   $self->{fault_strings}->{fault_noprivs}        = "Insufficient privileges.";
   $self->{fault_strings}->{fault_nosuch_key}     = "No such key.";
   $self->{fault_strings}->{fault_alreadyexists}  = "Item already exists.";
   $self->{fault_strings}->{fault_toomany}        = "Too many items.";
   $self->{fault_strings}->{fault_emailfail}      = "Email verification failed.";
   $self->{fault_strings}->{fault_notverified}    = "Action not verified.";
   $self->{fault_strings}->{fault_nochange}       = "No change.";
   $self->{fault_strings}->{fault_already_authed} = "You are already authenticated.";
   $self->{fault_strings}->{fault_unimplemented}  = "Method not implemented.";

   $self->call_svs ($args)
}

=head2 send

SEND allows you to send a memo to a nickname
that is offline at the moment.  When they
come online they will be told they have messages
waiting for them and will have an opportunity
to read your memo.
 
Your memo cannot be more than 300 characters.
 
Syntax: SEND <user|nick> text

=cut

sub send {
   my ($self, $args) = @_;

   $self->{cmd} = "send";

   # Change the default text for the dispatch table
   $self->{fault_strings}->{fault_needmoreparams} = "Insufficient parameters.";
   $self->{fault_strings}->{fault_badparams}      = "Invalid parameters.";
   $self->{fault_strings}->{fault_nosuch_source}  = "No such source.";
   $self->{fault_strings}->{fault_nosuch_target}  = "No such target.";
   $self->{fault_strings}->{fault_authfail}       = "Authentication failed.";
   $self->{fault_strings}->{fault_noprivs}        = "Insufficient privileges.";
   $self->{fault_strings}->{fault_nosuch_key}     = "No such key.";
   $self->{fault_strings}->{fault_alreadyexists}  = "Item already exists.";
   $self->{fault_strings}->{fault_toomany}        = "Too many items.";
   $self->{fault_strings}->{fault_emailfail}      = "Email verification failed.";
   $self->{fault_strings}->{fault_notverified}    = "Action not verified.";
   $self->{fault_strings}->{fault_nochange}       = "No change.";
   $self->{fault_strings}->{fault_already_authed} = "You are already authenticated.";
   $self->{fault_strings}->{fault_unimplemented}  = "Method not implemented.";

   $self->call_svs ($args)
}

=head1 AUTHORS

Author: Pippijn van Steenhoven <pip88nl@gmail.com>

This package is part of the Atheme IRC Services.

=cut

1
