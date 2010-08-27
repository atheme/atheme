package Atheme::OperServ;

use strict;
use warnings;
use utf8;

use base qw/Atheme/;

=head1 NAME

Atheme::OperServ - Perl interface to Atheme's XML-RPC methods

=head1 DESCRIPTION

This class provides an interface to the XML-RPC methods of the Atheme IRC Services.

=head1 METHODS

These are all either virtual or helper methods. They are being implemented in service-specific classes.

=head2 new

Services constructor. Takes a hash as argument:
   my $svs = new Atheme::OperServ(url => "http://localhost:8000");
 
url: URL for the XML-RPC server

lang: Language code (en, ...)

=cut

sub new {
   my ($self, %arg) = @_;

   # Call base constructor
   my $svs = $self->SUPER::new(%arg);

   $svs->{svs} = "OperServ";
   $svs
}

=head2 akill

AKILL allows you to maintain network-wide bans a la DALnet AKILL.
Services will keep your AKILLs stored and allow for easy management.
 

Syntax: AKILL ADD <nick|hostmask> [!P|!T <minutes>] <reason>
 
If the !P token is specified the AKILL will never expire (permanent).
If the !T token is specified expire time must follow, in minutes,
hours ("h"), days ("d") or weeks ("w").
 
The first example looks for the user with a nickname of "foo" and adds
a 5 minute AKILL for "bar reason."
 
The second example is similar but adds the AKILL for 3 days instead of
5 minutes.
 
The third example adds a permanent AKILL on foo@bar.com for "foo reason."
 
The fourth example adds a AKILL on foo@bar.com for the duration specified
in the configuration file for "foo reason."
 

Syntax: AKILL DEL <hostmask|number>
 
If number is specified it correlates with the number on AKILL LIST.
You may specify multiple numbers by separating with commas.
You may specify a range by using a colon.
 
 
Syntax: AKILL LIST [FULL]
 
If FULL is specified the AKILL reasons will be shown.
 

Syntax: AKILL SYNC
 
Sends all akills to all servers. This can be useful in case
services will be down or do not see a user as matching a
certain akill.

=cut

sub akill {
   my ($self, $args) = @_;

   $self->{cmd} = "akill";

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

=head2 clearchan

CLEARCHAN allows operators to clear
a channel in one of three ways: KICK,
which kicks all users from the channel, KILL,
which kills all users in the channel off the
network, or GLINE, which sets a one week
network ban against the hosts of all users in
the channel.
 
This command should not be used lightly.
 
Syntax: CLEARCHAN KICK|KILL|GLINE <#channel> <reason>

=cut

sub clearchan {
   my ($self, $args) = @_;

   $self->{cmd} = "clearchan";

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

=head2 clones

CLONES keeps track of the number of clients
per IP address. Warnings are displayed in
the snoop channel about IP addresses with
more than 3 clients, and network bans may
optionally be set on IP addresses with more
than 5 clients.
 
CLONES only works on clients whose IP address
Atheme knows. If the ircd does not support
propagating IP addresses at all, CLONES is
not useful; if IP addresses are not sent for
spoofed clients, those clients are exempt from
CLONES checking.
 
Syntax: CLONES KLINE ON|OFF
 
Enables/disables banning IP addresses with more
than 5 clients from the network for one hour
(these bans are not added to the AKILL list).
This setting is saved in etc/exempts.db and
defaults to off.
 
Syntax: CLONES LIST
 
Shows all IP addresses with more than 3 clients
with the number of clients and whether the IP
address is exempt.
 
Syntax: CLONES ADDEXEMPT <ip> <clones> <reason>
 
Adds an IP address to the clone exemption list.
The IP address must match exactly with the form
used by the ircd (mind '::' shortening with IPv6).
<clones> is the number of clones allowed; it must be
at least 4. Warnings are sent if this number is
exceeded, and a network ban may be set if the number
is exceeded by 10 or more.
The reason is shown in LISTEXEMPT.
The clone exemption list is stored in etc/exempts.db
and saved whenever it is modified.
 
Syntax: CLONES DELEXEMPT <ip>
 
Removes an IP address from the clone exemption list.
 
Syntax: CLONES LISTEXEMPT
 
Shows the clone exemption list with reasons.

=cut

sub clones {
   my ($self, $args) = @_;

   $self->{cmd} = "clones";

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

=head2 compare

COMPARE allows operators with chan:auspex
privilege to view matching information
on two users, or two channels.
 
It is useful in clone detection,
amongst other situations.
 
Syntax: COMPARE <#channel|user> <#channel|user>

=cut

sub compare {
   my ($self, $args) = @_;

   $self->{cmd} = "compare";

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

Services has an ignore list which functions similarly to
the way a user can ignore another user. If a user matches
a mask in the ignore list and attempts to use services,
they will not get a reply.
 
    ADD   - Add a mask to the ignore list.
    DEL   - Delete a mask from the ignore list.
    LIST  - List all the entries in the ignore list.
    CLEAR - Clear all the entries in the ignore list.

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

=head2 inject

INJECT fakes data from the uplink.  This
command is for debugging only and should
not be used unless you know what you're doing.

Syntax: INJECT <parameters>

=cut

sub inject {
   my ($self, $args) = @_;

   $self->{cmd} = "inject";

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

=head2 jupe

JUPE introduces a fake server with the given name, so
that the real server cannot connect. Jupes only last
as long as services is connected to the uplink and
can also (on most ircds) be removed with a simple
/squit command.

Syntax: JUPE <server> <reason>

=cut

sub jupe {
   my ($self, $args) = @_;

   $self->{cmd} = "jupe";

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

=head2 mode

MODE allows for the editing of modes on a channel. Some networks
will most likely find this command to be unethical.

Syntax: MODE <#channel> <mode> [parameters]

=cut

sub mode {
   my ($self, $args) = @_;

   $self->{cmd} = "mode";

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

=head2 modinspect

MODINSPECT displays detailed information about a module.

The names can be gathered from the MODLIST command. They
are not necessarily equal to the pathnames to load them
with MODLOAD.

Syntax: MODINSPECT <name>

=cut

sub modinspect {
   my ($self, $args) = @_;

   $self->{cmd} = "modinspect";

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

=head2 modlist

MODLIST displays a listing of all loaded
modules and their addresses.

=cut

sub modlist {
   my ($self, $args) = @_;

   $self->{cmd} = "modlist";

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

=head2 modload

MODLOAD loads one or more modules.

If the path does not start with a slash, it is taken
relative to PREFIX/modules or PREFIX/lib/atheme/modules
(depending on how Atheme was compiled). Specifying a
suffix like .so is optional.

Syntax: MODLOAD <path...>

=cut

sub modload {
   my ($self, $args) = @_;

   $self->{cmd} = "modload";

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

=head2 modrestart

MODRESTART unloads all non-permanent modules, then loads
all modules in the configuration file.

operserv/main and operserv/modrestart are also untouched.

A full restart may be more reliable than a module restart.

=cut

sub modrestart {
   my ($self, $args) = @_;

   $self->{cmd} = "modrestart";

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

=head2 modunload

MODUNLOAD unloads one or more modules. Not all modules
can be unloaded.

The names can be gathered from the MODLIST command. They
are not necessarily equal to the pathnames to load them
with MODLOAD.

Syntax: MODUNLOAD <name...>

=cut

sub modunload {
   my ($self, $args) = @_;

   $self->{cmd} = "modunload";

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

=head2 noop

NOOP allows you to deny IRCop access on a per-hostmask
or per-server basis. If a matching user opers up, they
will be killed.

Syntax: NOOP ADD HOSTMASK <nick!user@host> [reason]
Syntax: NOOP ADD SERVER <mask> [reason]

Syntax: NOOP DEL HOSTMASK <nick!user@host>
Syntax: NOOP DEL SERVER <mask>

=cut

sub noop {
   my ($self, $args) = @_;

   $self->{cmd} = "noop";

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

=head2 rakill

RAKILL allows for regex-based akills,
which are useful for removing clones
or botnets. The akills are not added
to OperServ's list and last a week.

Be careful, as regex is very easy to
make mistakes with. Use RMATCH first.
The regex syntax is exactly the same.

Syntax: RAKILL /<pattern>/[i] <reason>

=cut

sub rakill {
   my ($self, $args) = @_;

   $self->{cmd} = "rakill";

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

=head2 raw

RAW injects data into the uplink.  This
command is for debugging only and should
not be used unless you know what you're
doing.

Syntax: RAW <parameters>

=cut

sub raw {
   my ($self, $args) = @_;

   $self->{cmd} = "raw";

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

=head2 rehash

REHASH updates the database and reloads
the configuration file.  You can perform
a rehash from system console with a kill -HUP
command.

Syntax: REHASH

=cut

sub rehash {
   my ($self, $args) = @_;

   $self->{cmd} = "rehash";

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

=head2 restart

RESTART shuts down services and restarts them.

Syntax: RESTART

=cut

sub restart {
   my ($self, $args) = @_;

   $self->{cmd} = "restart";

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

=head2 rmatch

RMATCH shows all users whose
nick!user@host gecos
matches the given POSIX extended
regular expression.

Instead of a slash, any character that
is not a letter, digit, whitespace or
backslash and does not occur in the pattern
can be used. An i after the pattern means
case insensitive matching.

Syntax: RMATCH /<pattern>/[i]

=cut

sub rmatch {
   my ($self, $args) = @_;

   $self->{cmd} = "rmatch";

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

=head2 rnc

RNC shows the most common realnames on the network.

Syntax: RNC [number]

=cut

sub rnc {
   my ($self, $args) = @_;

   $self->{cmd} = "rnc";

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

=head2 rwatch

RWATCH maintains a list of regular expressions,
which the nick!user@host gecos
of all connecting clients are matched against.
Matching clients can be displayed in the snoop
channel and/or banned from the network. These
network bans are set on *@host, last 24 hours
and are not added to the AKILL list.
The RWATCH list is stored in etc/rwatch.db and
saved whenever it is modified.

See RMATCH for more information about regular
expression syntax.

Syntax: RWATCH ADD /<pattern>/[i] <reason>

Adds a regular expression to the RWATCH list.
The reason is shown in snoop notices and kline reasons.

Syntax: RWATCH DEL /<pattern>/[i]

Removes a regular expression from the RWATCH list.

Syntax: RWATCH LIST

Shows the RWATCH list. The meaning of the letters is:
    i - case insensitive match
    S - matching clients are shown in the snoop channel
    K - matching clients are banned from the network

Syntax: RWATCH SET /<pattern>/[i] <options>

Changes the action for a regular expression. Possible
values for <options> are:
    SNOOP   - enables display in the snoop channel
    NOSNOOP - disables display in the snoop channel
    KLINE   - enables network bans
    NOKLINE - disables network bans

=cut

sub rwatch {
   my ($self, $args) = @_;

   $self->{cmd} = "rwatch";

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

=head2 shutdown

SHUTDOWN shuts down services. Services will
not reconnect or restart.

Syntax: SHUTDOWN

=cut

sub shutdown {
   my ($self, $args) = @_;

   $self->{cmd} = "shutdown";

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

=head2 soper

SOPER allows manipulation of services operator privileges.

SOPER LIST shows all accounts with services operator
privileges, both from the configuration file and the
this command. It is similar to /stats o &nick&.

SOPER LISTCLASS shows all defined oper classes. Use
the SPECS command to view the privileges associated
with an oper class.

SOPER ADD grants services operator privileges to an
account. The granted privileges are described by an
oper class.

SOPER DEL removes services operator privileges from
an account.

It is not possible to modify accounts with
operator{} blocks in the configuration file.

Syntax: SOPER LIST|LISTCLASS
Syntax: SOPER ADD <account> <operclass>
Syntax: SOPER DEL <account>

=cut

sub soper {
   my ($self, $args) = @_;

   $self->{cmd} = "soper";

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

=head2 specs

SPECS shows the privileges you have in services.

Syntax: SPECS

It is also possible to see the privileges of other
online users or of oper classes.

Syntax: SPECS USER <nick>
Syntax: SPECS OPERCLASS <classname>

=cut

sub specs {
   my ($self, $args) = @_;

   $self->{cmd} = "specs";

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

=head2 update

UPDATE flushes the database to disk.

Syntax: UPDATE

=cut

sub update {
   my ($self, $args) = @_;

   $self->{cmd} = "update";

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

=head2 uptime

UPTIME shows services uptime and the number of
registered nicks and channels.

=cut

sub uptime {
   my ($self, $args) = @_;

   $self->{cmd} = "uptime";

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
