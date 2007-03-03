package Atheme;

use strict;
use warnings;
use utf8;
use vars qw($ERROR);

use Carp;

our $VERSION = "0.01";
$ERROR = '';

require RPC::XML;
require RPC::XML::Client;

=head1 NAME

Atheme - Perl interface to Atheme's XML-RPC methods

=head1 DESCRIPTION

This class provides an interface to the XML-RPC methods of the Atheme IRC Services.

=head1 METHODS

These are all either virtual or helper methods. They are being implemented in
service-specific classes.

=head2 new

Services constructor. Takes a hash as argument:
   my $svs = new Atheme(url => "http://localhost:8000");

url<string>: URL to Atheme's XML-RPC server.

lang<string>: Language for result strings (en, ...).

validate<boolean>: If 1 then validation should be done in perl already, if 0
then validation is only done in atheme itself. In both cases, atheme
validates. If you choose to use the perl validation, you will get more verbose
fault messages containing an additional key 'subtype'.

=cut

sub new {
   my ($self, %arg) = @_;

   # There is no default for url
   my $url = delete $arg{url}
      or croak "Atheme: You need to provide an url";

   # Some default values
   my $svs = bless {
      rpc      => RPC::XML::Client->new($url),
      lang     => 'en',
      validate => 1,
      %arg,
   }, $self;

   require FormValidator::Simple
      if $svs->{validate};

   # Get fault and success handling dispatch tables
   $svs->init_dispatcher;

   $svs
}

=head2 init_dispatcher

Initialises the dispatch tables handling faults or success

Here is the list of fault types and default strings (these are likely to be
at least partially overridden by Atheme::*Serv classes.

   fault_needmoreparams = "Insufficient parameters."
   fault_badparams      = "Invalid parameters."
   fault_nosuch_source  = "No such source."
   fault_nosuch_target  = "No such target."
   fault_authfail       = "Authentication failed."
   fault_noprivs        = "Insufficient privileges."
   fault_nosuch_key     = "No such key."
   fault_alreadyexists  = "Item already exists."
   fault_toomany        = "Too many items."
   fault_emailfail      = "Email verification failed."
   fault_notverified    = "Action not verified."
   fault_nochange       = "No change."
   fault_already_authed = "You are already authenticated."
   fault_unimplemented  = "Method not implemented."


=cut

sub init_dispatcher {
   my ($self) = @_;

   # Default string table for the fault dispatch table (should be overridden per method)
   $self->{fault_strings} = {
      fault_needmoreparams => "Insufficient parameters.",
      fault_badparams      => "Invalid parameters.",
      fault_nosuch_source  => "No such source.",
      fault_nosuch_target  => "No such target.",
      fault_authfail       => "Authentication failed.", 
      fault_noprivs        => "Insufficient privileges.",
      fault_nosuch_key     => "No such key.",
      fault_alreadyexists  => "Item already exists.", 
      fault_toomany        => "Too many items.",
      fault_emailfail      => "Email verification failed.",
      fault_notverified    => "Action not verified.", 
      fault_nochange       => "No change.",
      fault_already_authed => "You are already authenticated.", 
      fault_unimplemented  => "Method not implemented."
   };

   # Fault dispatch table
   $self->{fault_dispatch} = {
      1  => sub { $self->{result} = { type => 'fault_needmoreparams', string => $self->{fault_strings}->{fault_needmoreparams} }; },
      2  => sub { $self->{result} = { type => 'fault_badparams',      string => $self->{fault_strings}->{fault_badparams}      }; },
      3  => sub { $self->{result} = { type => 'fault_nosuch_source',  string => $self->{fault_strings}->{fault_nosuch_source}  }; },
      4  => sub { $self->{result} = { type => 'fault_nosuch_target',  string => $self->{fault_strings}->{fault_nosuch_target}  }; },
      5  => sub { $self->{result} = { type => 'fault_authfail',       string => $self->{fault_strings}->{fault_authfail}       }; },
      6  => sub { $self->{result} = { type => 'fault_noprivs',        string => $self->{fault_strings}->{fault_noprivs}        }; },
      7  => sub { $self->{result} = { type => 'fault_nosuch_key',     string => $self->{fault_strings}->{fault_nosuch_key}     }; },
      8  => sub { $self->{result} = { type => 'fault_alreadyexists',  string => $self->{fault_strings}->{fault_alreadyexists}  }; },
      9  => sub { $self->{result} = { type => 'fault_toomany',        string => $self->{fault_strings}->{fault_toomany}        }; },
      10 => sub { $self->{result} = { type => 'fault_emailfail',      string => $self->{fault_strings}->{fault_emailfail}      }; },
      11 => sub { $self->{result} = { type => 'fault_notverified',    string => $self->{fault_strings}->{fault_notverified}    }; },
      12 => sub { $self->{result} = { type => 'fault_nochange',       string => $self->{fault_strings}->{fault_nochange}       }; },
      13 => sub { $self->{result} = { type => 'fault_already_authed', string => $self->{fault_strings}->{fault_already_authed} }; },
      14 => sub { $self->{result} = { type => 'fault_unimplemented',  string => $self->{fault_strings}->{fault_unimplemented}  }; },
   };

   # Result dispatch table
   $self->{dispatch} = {
      'RPC::XML::fault'  => sub { $self->{fault_dispatch}->{$self->{result}->code}->(); },
      'RPC::XML::string' => sub { $self->{result} = { type => 'success',    string => $self->{result}->value }; },
      'other'            => sub { $self->{result} = { type => 'fault_http', string => "Connection refused." }; }
   };

   ()
}

=head2 call_rpc

Method call

=cut

sub call_rpc {
   my ($self) = @_;

   $self->{result} = $self->{rpc}->send_request('atheme.command',
      $self->{authcookie} || "x",
      $self->{nick} || "x",
      $self->{address} || "x",
      $self->{svs} || "x",
      $self->{cmd} || "x",
      $self->{params});

   eval { print "--------------------------------------------------------\nOriginal          | " . $self->{result}->string . "\n"; };

   $self->{dispatch}{ref $self->{result} || 'other'}->()
}

=head2 call_svs

Call service command

Required variables:

   $self->{authcookie}
   $self->{nick}
   $self->{address}
   $self->{svs}
   $self->{cmd}
   $self->{params}

=cut

sub call_svs {
   my ($self, $args) = @_;

   # Set required variables
   $self->{authcookie} = $args->{authcookie};
   $self->{nick} = $args->{nick};
   $self->{address} = $args->{address};
   $self->{params} = $args->{params};

   # Make the call
   $self->call_rpc;

   # Return the result, or the fault
   $self->{result}
}

=head2 login

A common method used to log into the services in order to execute other
methods. Every service inherits this, so you can load just Atheme::MemoServ
and log in through that.

=cut

sub login {
   my ($self, $args) = @_;

   # Set required variables
   $self->{nick} = $args->{nick};
   $self->{pass} = $args->{pass};
   $self->{address} = $args->{address};

   # This method is different from all others and so has to be called separately.
   $self->{result} = $self->{rpc}->send_request('atheme.login',
      $self->{nick},
      $self->{pass},
      $self->{address});

   # Dispatch table changes for login
   $self->{fault_strings}->{fault_needmoreparams} = "Insufficient parameters.";
   $self->{fault_strings}->{fault_nosuch_source}  = "The account is not registered.",
   $self->{fault_strings}->{fault_authfail}       = "The password is not valid for this account.";
   $self->{fault_strings}->{fault_noprivs}        = "The account has been frozen.";

   $self->{dispatch}{ref $self->{result} || 'other'}->()
}

=head2 logout

A common method used to log out and clean up your authcookie. This should be
done, but does not have to be done. This method is also inherited and
therefore usable in every *Serv.

=cut

sub logout {
   my ($self, $args) = @_;

   # Set required variables
   $self->{authcookie} = $args->{authcookie};
   $self->{nick} = $args->{nick};

   # This method is different from all others and so has to be called separately.
   $self->{result} = $self->{rpc}->send_request('atheme.logout',
      $self->{authcookie},
      $self->{nick});

   # Dispatch table changes for login
   $self->{fault_strings}->{fault_nosuch_source}  = "Unknown user.";
   $self->{fault_strings}->{fault_authfail}       = "Invalid authcookie for this account.";

   $self->{dispatch}{ref $self->{result} || 'other'}->()
}

=head1 AUTHORS

Author: Pippijn van Steenhoven <pip88nl@gmail.com>

This package is part of the Atheme IRC Services.

=cut

1
