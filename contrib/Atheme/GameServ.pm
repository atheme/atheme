package Atheme::GameServ;

use strict;
use warnings;
use utf8;

use base qw/Atheme/;

=head1 NAME

Atheme::GameServ - Perl interface to Atheme's XML-RPC methods

=head1 DESCRIPTION

This class provides an interface to the XML-RPC methods of the Atheme IRC Services.

=head1 METHODS

These are all either virtual or helper methods. They are being implemented in service-specific classes.

=head2 new

Services constructor. Takes a hash as argument:
   my $svs = new Atheme::GameServ(url => "http://localhost:8000");

=cut

sub new {
   my ($self, %arg) = @_;

   # Call base constructor
   my $svs = $self->SUPER::new(%arg);

   $svs->{svs} = "GameServ";
   $svs
}

=head2 dice

ROLL            Rolls one or more dice.

=cut

sub dice {
   my ($self, $args) = @_;

   $self->{cmd} = "dice";

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

=head2 eightball

EIGHTBALL       Ask the 8-Ball a question.

=cut

sub eightball {
   my ($self, $args) = @_;

   $self->{cmd} = "eightball";

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

=head2 namegen

NAMEGEN         Generates some names to ponder.

=cut

sub namegen {
   my ($self, $args) = @_;

   $self->{cmd} = "namegen";

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

=head2 rps

RPS             Rock, Paper, Scissors

=cut

sub rps {
   my ($self, $args) = @_;

   $self->{cmd} = "rps";

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
