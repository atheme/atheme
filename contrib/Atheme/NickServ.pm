package Atheme::NickServ;

use strict;
use warnings;
use utf8;

use FormValidator::Simple;

use base qw/Atheme/;

=head1 NAME

Atheme::NickServ - Perl interface to Atheme's XML-RPC methods

=head1 DESCRIPTION

This class provides an interface to the XML-RPC methods of the Atheme IRC Services.

=head1 METHODS

These are all either virtual or helper methods. They are being implemented in service-specific classes.

=head2 new

Services constructor. Takes a hash as argument:
   my $svs = new Atheme::NickServ(url => "http://localhost:8000");

=cut

sub new {
   my ($self, %arg) = @_;

   # Call base constructor
   my $svs = $self->SUPER::new(%arg);

   $svs->{svs} = "NickServ";
   $svs
}

=head2 access

ACCESS maintains a list of user@host masks from where
NickServ will recognize you, so it will not prompt you to
change nick or identify. Getting channel access or
editing nickname settings still requires identification,
however. Also, you can only be recognized as the nick you
are currently using.
 
Access list entries can use hostnames with optional
wildcards, IP addresses and CIDR masks. There are
restrictions on how much you can wildcard.

Syntax: ACCESS LIST
Syntax: ACCESS ADD <mask>
Syntax: ACCESS DEL <mask>

=cut

sub access {
   my ($self, $args) = @_;

   # Validate user input
   if ($self->{validate}) {
      my $query = {
         cmd => lc $args->{params}->[0],
         mask => lc $args->{params}->[1] || "",
      };

      # TODO: Regexes for invalid or too wide masks...
      my $result = FormValidator::Simple->check( $query => [
         { cmd_blank     => 'cmd' } => ['NOT_BLANK'],
         { mask_invalid  => 'mask' } => [['REGEX', qr/^[^!@]+@[^!@]+$/], ['REGEX', qr/@[^\.]+\./], ['NOT_REGEX', qr/\.\./]],
         { mask_too_wide => 'mask' } => [['NOT_REGEX', qr/\*(.+)?@(.+)?\*(.+)?/]],
         ]);
      
      if ($result->has_error) {
         my $subdispatcher = {
            cmd_blank     => { type => 'fault_needmoreparams', string => 'Please provide a subcommand.' },
            mask_invalid  => { type => 'fault_badparams',      string => 'Invalid mask.' },
            mask_too_wide => { type => 'fault_badparams',      string => 'Mask too wide.' },
         };

         $result->error($_)
            ? $self->{result} = { type => $subdispatcher->{$_}->{type}, subtype => $_, string => $subdispatcher->{$_}->{string} }
            : ()
               for keys %$subdispatcher;

         return $self->{result};
      }
   }

   $self->{cmd} = "ACCESS";

   # Change the default text for the dispatch table
   $self->{fault_strings}->{fault_badparams}      = $args->{params}->[1] . " is not registered.";
   $self->{fault_strings}->{fault_noprivs}        = "You are not authorized to use the target argument.";
   $self->{fault_strings}->{fault_toomany}        = "Your access list is full.";
   $self->{fault_strings}->{fault_nochange}       = "Mask " . $args->{params}->[1] . " is " . (lc $args->{params}->[0] eq "add" ? "already" : "not") . " on your access list.";

   $self->call_svs ($args)
}

=head2 drop

Using this command makes NickServ remove your account
and stop watching your nick(s), If a nick is dropped,
anyone else can register it. You will also lose all
your channel access and memos. You must use the
NickServ IDENTIFY command before doing this.
 
When dropping and re-registering an account during a
netsplit, users on the other side of the split may later
be recognized as the new account.

Syntax: DROP <nickname> <password>

=cut

sub drop {
   my ($self, $args) = @_;

   $self->{cmd} = "DROP";

   # Change the default text for the dispatch table
   $self->{fault_strings}->{fault_needmoreparams} = "Please enter a nick you want to drop.";
   $self->{fault_strings}->{fault_badparams}      = "Invalid parameters.";
   $self->{fault_strings}->{fault_nosuch_target}  = $args->{params}->[0] . " is not registered or is a grouped nickname.";
   $self->{fault_strings}->{fault_authfail}       = "Authentication failed. Invalid password for " . $args->{params}->[0];
   $self->{fault_strings}->{fault_noprivs}        = "You are not allowed to drop " . $args->{params}->[0] . ".",

   $self->call_svs ($args)
}

=head2 freeze

FREEZE allows operators to "freeze" an abusive user's
account. Users cannot log into a frozen account. Thus,
users cannot obtain the access associated with the
account.
 
FREEZE information will be displayed in INFO output.

Syntax: FREEZE <nick> ON|OFF <reason>

=cut

sub freeze {
   my ($self, $args) = @_;

   $self->{cmd} = "freeze";

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

=head2 ghost

GHOST disconnects an old user session, or somebody
attempting to use your nickname without authorization.
 
If you are logged in to the nick's account, you need
not specify a password, otherwise you have to.

Syntax: GHOST <nick> [password]

=cut

sub ghost {
   my ($self, $args) = @_;

   $self->{cmd} = "ghost";

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

=head2 group

GROUP registers your current nickname to your account.
This means that NickServ protects this nickname the
same way as it protects your account name. Most
services commands will accept the new nickname as
an alias for your account name.
 
Please note that grouped nicks expire separately
from accounts. To prevent this, you must use them.
Otherwise, all properties of the account are shared
among all nicks registered to it.

Syntax: GROUP

=cut

sub group {
   my ($self, $args) = @_;

   $self->{cmd} = "group";

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

HOLD prevents an account and all nicknames registered
to it from expiring.

Syntax: HOLD <nick> ON|OFF

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

INFO displays account information such as
registration time, flags, and other details.
Additionally it will display registration
and last seen time of the nick you give.
 
You can query the nick a user is logged in as
by specifying an equals sign followed by their
nick. This '=' convention works with most commands.

Syntax: INFO <nickname>
Syntax: INFO =<online user>

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

=head2 list

LIST shows nicknames that match a given
pattern. Wildcards are allowed.
 
If user@host is also given, only nicknames
whose last seen host matches it are shown.

Syntax: LIST <pattern>
Syntax: LIST <pattern>!<user@host>

=cut

sub list {
   my ($self, $args) = @_;

   $self->{cmd} = "LIST";

   $self->{fault_strings}->{fault_needmoreparams} = "Please provide a pattern to match nicknames.";

   $self->call_svs ($args)
}

=head2 listchans

LISTCHANS shows the channels that you have access
to, including those that you own.
 
AKICKs and host-based access are not shown.

Syntax: LISTCHANS

=cut

sub listchans {
   my ($self, $args) = @_;

   $self->{cmd} = "listchans";

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

=head2 listmail

LISTMAIL shows nicknames registered to a
given e-mail address. Wildcards are allowed.

Syntax: LISTMAIL <email>

=cut

sub listmail {
   my ($self, $args) = @_;

   $self->{cmd} = "listmail";

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

MARK allows operators to attach a note to an account.
For example, an operator could mark the account of
a spammer so that others know the user has previously
been warned.

Syntax: MARK <nickname> ON|OFF <reason>

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

=head2 register

This will register your current nickname with NickServ.
This will allow you to assert some form of identity on
the network and to be added to access lists. Furthermore,
NickServ will warn users using your nick without
identifying and allow you to kill ghosts.
The password is a case-sensitive password that you make
up. Please write down or memorize your password! You
will need it later to change settings.
 
You may be required to confirm the email address. To do this,
follow the instructions in the message sent to the email
address.

Syntax: REGISTER <password> <email-address>

=cut

sub register {
   my ($self, $args) = @_;

   # Validate user input
   if ($self->{validate}) {
      my $query = {
         nick => lc $args->{params}->[0],
         pass => lc $args->{params}->[1],
         mail => $args->{params}->[2]
      };

      my $result = FormValidator::Simple->check( $query => [
         { nick_blank => ['nick'] } => ['NOT_BLANK'],
         { pass_blank => ['pass'] } => ['NOT_BLANK'],
         { mail_blank => ['mail'] } => ['NOT_BLANK'],
         { nick_len   => ['nick'] } => [['LENGTH', 3, 50]],
         { pass_len   => ['pass'] } => [['LENGTH', 5, 32]],
         { mail_len   => ['mail'] } => [['LENGTH', 5, 119]],
         { pass_nick  => ['pass', 'nick'] } => ['NOT_DUPLICATION'],
         { mail_mx    => ['mail'] } => ['EMAIL_MX'],
         ]);

      if ($result->has_error) {
         my $subdispatcher = {
            nick_blank => { type => 'fault_needmoreparams', string => 'Please provide a nickname.' },
            pass_blank => { type => 'fault_needmoreparams', string => 'Please provide a password.' },
            mail_blank => { type => 'fault_needmoreparams', string => 'Please provide an email address.' },
            nick_len   => { type => 'fault_badparams',      string => 'Your nickname should be between 3 and 50 characters.' },
            pass_len   => { type => 'fault_badparams',      string => 'Your password should be between 5 and 32 characters.' },
            mail_len   => { type => 'fault_badparams',      string => 'Your email address should be between 5 and 119 characters.' },
            pass_nick  => { type => 'fault_badparams',      string => 'You cannot use your nickname as a password.' },
            mail_mx    => { type => 'fault_badparams',      string => 'Please provide a valid email address.' },
         };

         $result->invalid($_)
            ? $self->{result} = { type => $subdispatcher->{$_}->{type}, subtype => $_, string => $subdispatcher->{$_}->{string} }
            : ()
               for keys %$subdispatcher;

         return $self->{result};
      }
   }
   
   $self->{cmd} = "REGISTER";

   # Change the default text for the dispatch table
   $self->{fault_strings}->{fault_needmoreparams} = "Insufficient parameters for REGISTER.";
   $self->{fault_strings}->{fault_badparams}      = "Invalid parameters.";
   $self->{fault_strings}->{fault_noprivs}        = "A user matching this account is already on IRC.";
   $self->{fault_strings}->{fault_alreadyexists}  = $args->{params}->[0] . " is already registered.";
   $self->{fault_strings}->{fault_toomany}        = $args->{params}->[2] . " has too many nicknames registered.";
   $self->{fault_strings}->{fault_emailfail}      = "Sending email failed, sorry! Registration aborted.";
   $self->{fault_strings}->{fault_already_authed} = "You are already logged in.";

   $self->call_svs ($args)
}

=head2 resetpass

RESETPASS sets a random password for the specified
account.

Syntax: RESETPASS <nickname>

=cut

sub resetpass {
   my ($self, $args) = @_;

   $self->{cmd} = "resetpass";

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

=head2 return

RETURN resets the specified account
password and sends it to the email address
specified.
 
Syntax: RETURN <nickname> <e-mail>

=cut

sub return {
   my ($self, $args) = @_;

   $self->{cmd} = "return";

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

=head2 sendpass

SENDPASS emails the password for the specified
nickname to the corresponding email address.
 
Syntax: SENDPASS <nickname>

=cut

sub sendpass {
   my ($self, $args) = @_;

   $self->{cmd} = "sendpass";

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
for nicknames that change the way certain operations
are performed on them.
 
The following subcommands are available:
EMAIL           Changes your e-mail address.
EMAILMEMOS      Forwards incoming memos to your e-mail address.
HIDEMAIL        Hides your e-mail address.
NOMEMO          Disables the ability to recieve memos.
NOOP            Prevents services from setting modes upon you automatically.
NEVEROP         Prevents you from being added to access lists.
PASSWORD        Changes the password associated with your nickname.
PROPERTY        Manipulates metadata entries associated with a nickname.
ENFORCE         Enables or disables automatic protection of a nickname.

Syntax: SET <subcommand> ON|OFF
Syntax: SET PROPERTY <key> <value>

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
 
Syntax: STATUS

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
with registered users.

Syntax: TAXONOMY <nick>
 
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

=head2 verify

VERIFY confirms a change associated with
your account registration.
 
Syntax: VERIFY <operation> <nickname> <key>

=cut

sub verify {
   my ($self, $args) = @_;

   $self->{cmd} = "verify";

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

=head2 vhost

VHOST allows operators to set a virtual host (also
known as a spoof or cloak) on an account. This vhost
will be set on the user immediately and each time
they identify. If no vhost is specified, clear any
existing vhost.
 
Syntax: VHOST <nickname> [<vhost>]

=cut

sub vhost {
   my ($self, $args) = @_;

   $self->{cmd} = "vhost";

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
