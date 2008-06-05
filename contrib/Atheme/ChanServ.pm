package Atheme::ChanServ;

use strict;
use warnings;
use utf8;

use base qw/Atheme/;

=head1 NAME

Atheme::ChanServ - Perl interface to Atheme's XML-RPC methods

=head1 DESCRIPTION

This class provides an interface to the XML-RPC methods of the Atheme IRC Services.

=head1 METHODS

These are all either virtual or helper methods. They are being implemented in service-specific classes.

=head2 new

Services constructor. Takes a hash as argument:
   my $svs = new Atheme::ChanServ(url => "http://localhost:8000");

=cut

sub new {
   my ($self, %arg) = @_;

   # Call base constructor
   my $svs = $self->SUPER::new(%arg);

   $svs->{svs} = "ChanServ";
   $svs
}

=head2 akick

The AKICK command allows you to maintain channel
ban lists.  Users on the AKICK list will be
automatically kickbanned when they join the channel,
removing any matching ban exceptions first. Users
with the +r flag are exempt.
 
You may also specify a hostmask (nick!user@host)
for the AKICK list.
 
Removing an entry from the AKICK list will not
remove any channel bans placed by it.
 
Syntax: AKICK <#channel> ADD|DEL|LIST <nickname|hostmask>

=cut

sub akick {
   my ($self, $args) = @_;

   $self->{cmd} = "akick";

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

=head2 ban

The BAN command allows you to ban a user or hostmask from
a channel.
 
Syntax: BAN <#channel> <nickname|hostmask>

=cut

sub ban {
   my ($self, $args) = @_;

   $self->{cmd} = "ban";

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

=head2 clear

CLEAR allows you to clear various aspects of a channel.
 
The following subcommands are available:
BANS            Clears bans or other lists of a channel.
USERS           Kicks all users from a channel.

=cut

sub clear {
   my ($self, $args) = @_;

   $self->{cmd} = "clear";

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

=head2 close

CLOSE prevents a channel from being used. Anyone
who enters is immediately kickbanned. The channel
cannot be dropped and foundership cannot be
transferred.
 
Enabling CLOSE will immediately kick all
users from the channel.
 
Use CLOSE OFF to reopen a channel. While closed,
channels will still expire.
 
Syntax: CLOSE <#channel> ON|OFF [reason]

=cut

sub close {
   my ($self, $args) = @_;

   $self->{cmd} = "close";

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

=head2 count

This will give a count of how many entries are in each of
the channel's xOP lists and how many entries on the access
list do not match a xOP value (except the founder).
 
The second line shows how many access entries have each flag.
 
Syntax: COUNT <#channel>

=cut

sub count {
   my ($self, $args) = @_;

   $self->{cmd} = "count";

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

=head2 drop

DROP allows you to "unregister" a registered channel.
 
Once you DROP a channel all of the data associated
with it (access lists, etc) are removed and cannot
be restored.
 
Syntax: DROP <#channel>

=cut

sub drop {
   my ($self, $args) = @_;

   $self->{cmd} = "drop";

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

=head2 fflags

The FFLAGS command allows an oper to force a flags
change on a channel. The syntax is the same as FLAGS,
except that the target and flags changes must be given.
Viewing any channel's access list is done with FLAGS.
 
Syntax: FFLAGS <#channel> <nickname|hostmask> <template>
Syntax: FFLAGS <#channel> <nickname|hostmask> <flag_changes>

=cut

sub fflags {
   my ($self, $args) = @_;

   $self->{cmd} = "fflags";

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

=head2 flags

The FLAGS command allows for the granting/removal of channel
privileges on a more specific, non-generalized level. It
supports both nicknames and hostmasks as targets.
 
When only the channel argument is given, a listing of
permissions granted to users will be displayed.
 
Syntax: FLAGS <#channel>
 
Otherwise, an access entry is modified. A modification may be
specified by a template name (changes the access to the
template) or a flags change (starts with + or -). See the
TEMPLATE help entry for more information about templates.
 
If you are not the founder, you may only manipulate flags you
have yourself, and may not edit users that have flags you
don't have. For this purpose, +v grants +V, +h grants +H,
+o grants +O and +r grants +b.
 
If you do not have +f you may still remove your own access
with -*.
Syntax: FLAGS <#channel> [nickname|hostmask template]
Syntax: FLAGS <#channel> [nickname|hostmask flag_changes]
 
Permissions:
    +v - Enables use of the voice/devoice commands.
    +V - Enables automatic voice.
    +h - Enables use of the halfop/dehalfop commands.
    +H - Enables automatic halfop.
    +o - Enables use of the op/deop commands.
    +O - Enables automatic op.
    +s - Enables use of the set command.
    +i - Enables use of the invite and getkey commands.
    +r - Enables use of the kick, ban, and kickban commands.
    +R - Enables use of the recover and clear commands.
    +f - Enables modification of channel access lists.
    +t - Enables use of the topic and topicappend commands.
    +A - Enables viewing of channel access lists.
    +b - Enables automatic kickban.
 
The special permission +* adds all permissions except +b.
The special permission -* removes all permissions including +b.

=cut

sub flags {
   my ($self, $args) = @_;

   $self->{cmd} = "flags";

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

=head2 ftransfer

FTRANSFER forcefully transfers foundership
of a channel.
 
Syntax: FTRANSFER <#channel> <founder>

=cut

sub ftransfer {
   my ($self, $args) = @_;

   $self->{cmd} = "ftransfer";

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

=head2 getkey

GETKEY returns the key (+k, password to be allowed in)
of the specified channel: /join #channel key
 
Syntax: GETKEY <#channel>

=cut

sub getkey {
   my ($self, $args) = @_;

   $self->{cmd} = "getkey";

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

=head2 halfop

These commands perform status mode changes on a channel.
 
If you perform an operation on another user, they will be
notified that you did it.
 
If the last parameter is omitted the action is performed
on the person requesting the command.
 
Syntax: HALFOP|DEHALFOP <#channel> [nickname]

=cut

sub halfop {
   my ($self, $args) = @_;

   $self->{cmd} = "halfop";

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

=head2 hold

HOLD prevents a channel from expiring for inactivity.
Held channels will still expire when there are no
eligible successors.
 
Syntax: HOLD <#channel> ON|OFF

=cut

sub hold {
   my ($self, $args) = @_;

   $self->{cmd} = "hold";

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

=head2 info

INFO displays channel information such as
registration time, flags, and other details.
 
Syntax: INFO <#channel>

=cut

sub info {
   my ($self, $args) = @_;

   $self->{cmd} = "info";

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

=head2 invite

INVITE requests services to invite you to the
specified channel. This is useful if you use
the +i channel mode.
 
Syntax: INVITE <#channel>

=cut

sub invite {
   my ($self, $args) = @_;

   $self->{cmd} = "invite";

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

=head2 kick

The KICK command allows for the removal of a user from
a channel. The user can immediately rejoin.
 
Your nick will be added to the kick reason.
 
Syntax: KICK <#channel> <nick> [reason]

=cut

sub kick {
   my ($self, $args) = @_;

   $self->{cmd} = "kick";

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

LIST shows channels that match a given
pattern. Wildcards are allowed.
 
Syntax: LIST <pattern>

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

=head2 mark

MARK allows operators to attach a note to a channel.
For example, an operator could mark the channel of a
spammer so that others know it has previously been
warned.
 
MARK information will be displayed in INFO output.
 
Syntax: MARK <#channel> ON|OFF <reason>

=cut

sub mark {
   my ($self, $args) = @_;

   $self->{cmd} = "mark";

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

=head2 op

These commands perform status mode changes on a channel.
 
If you perform an operation on another user, they will be
notified that you did it.
 
If the last parameter is omitted the action is performed
on the person requesting the command.
 
Syntax: OP|DEOP <#channel> [nickname]

=cut

sub op {
   my ($self, $args) = @_;

   $self->{cmd} = "op";

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

=head2 recover

RECOVER allows you to regain control of your
channel in the event of a takeover.
 
More precisely, everyone will be deopped,
limit and key will be cleared, all bans
matching you are removed, a ban exception
matching you is added (in case of bans Atheme
can't see), the channel is set invite-only
and moderated and you are invited.
 
If you are on channel, you will be opped and
no ban exception will be added.
 
Syntax: RECOVER <#channel>

=cut

sub recover {
   my ($self, $args) = @_;

   $self->{cmd} = "recover";

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

=head2 register

REGISTER allows you to register a channel
so that you have better control. Registration
allows you to maintain a channel access list
and other functions that are normally
provided by IRC bots.
 
Syntax: REGISTER <#channel>

=cut

sub register {
   my ($self, $args) = @_;

   $self->{cmd} = "register";

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

=head2 set

SET allows you to set various control flags
for channels that change the way certain
operations are performed on them.
 
The following subcommands are available:
FOUNDER         Transfers foundership of a channel.
MLOCK           Sets channel mode lock.
SECURE          Prevents unauthorized users from gaining operator status.
VERBOSE         Notifies channel about access list modifications.
URL             Sets the channel URL.
ENTRYMSG        Sets the channel's entry message.
PROPERTY        Manipulates channel metadata.
EMAIL           Sets the channel e-mail address.
KEEPTOPIC       Enables topic retention.
TOPICLOCK       Restricts who can change the topic.
GUARD           Sets whether or not services will inhabit the channel.
FANTASY         Allows or disallows in-channel commands.
RESTRICTED      Restricts access to the channel to users on the access list. (Other users are kickbanned.)

=cut

sub set {
   my ($self, $args) = @_;

   $self->{cmd} = "set";

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

=head2 status

STATUS returns information about your current
state. It will show information about your
nickname, IRC operator, and SRA status.
 
If the a channel parameter is specified, your
access to the given channel is returned.
 
Syntax: STATUS [#channel]

=cut

sub status {
   my ($self, $args) = @_;

   $self->{cmd} = "status";

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

=head2 taxonomy

The taxonomy command lists metadata information associated
with registered channels.

Syntax: TAXONOMY <#channel>

=cut

sub taxonomy {
   my ($self, $args) = @_;

   $self->{cmd} = "taxonomy";

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

=head2 template

The TEMPLATE command allows definition of sets of flags,
simplifying the use of the FLAGS command.
 
Without arguments, network wide templates are shown.
These include at least SOP/AOP/HOP/VOP.
 
Syntax: TEMPLATE
 
When given only the channel argument, a listing of
templates for the channel will be displayed.

Syntax: TEMPLATE <#channel>
 
Otherwise, a template is modified. A modification may be
specified by a template name (copies the template) or a
flags change (starts with + or -, optionally preceded by
an !). Templates cannot be the empty set (making a
template empty deletes it).
 
If the ! form is used, all access entries which exactly
match the template are changed accordingly, with the
exception that it is not possible to remove the
founder's +f access.
 
There is a limit on the length of all templates on a
channel.
 
If you are not the founder, similar restrictions apply
as in FLAGS.
 
Syntax: TEMPLATE <#channel> [template oldtemplate]
Syntax: TEMPLATE <#channel> [template flag_changes]
Syntax: TEMPLATE <#channel> [template !flag_changes]

=cut

sub template {
   my ($self, $args) = @_;

   $self->{cmd} = "template";

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

=head2 topic

The TOPIC command allows for the changing of a topic on a channel.
 
Syntax: TOPIC <#channel> <topic>

=cut

sub topic {
   my ($self, $args) = @_;

   $self->{cmd} = "topic";

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

=head2 voice

These commands perform status mode changes on a channel.
 
If you perform an operation on another user, they will be
notified that you did it.
 
If the last parameter is omitted the action is performed
on the person requesting the command.
 
Syntax: VOICE|DEVOICE <#channel> [nickname]

=cut

sub voice {
   my ($self, $args) = @_;

   $self->{cmd} = "voice";

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

=head2 why

The WHY command shows the access entries an online
user matches.
 
Syntax: WHY <#channel> <nickname>

=cut

sub why {
   my ($self, $args) = @_;

   $self->{cmd} = "why";

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
