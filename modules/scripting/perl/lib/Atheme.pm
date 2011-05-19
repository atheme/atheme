package Atheme;

use Carp;
use Exporter 'import';

require DynaLoader;
our @ISA = qw(DynaLoader);

our @EXPORT = qw( %Services %Users %ChannelRegistrations %Hooks &depends );

bootstrap Atheme;

use Atheme::Fault;
use Atheme::Service;
use Atheme::Account;
use Atheme::ChannelRegistration;
use Atheme::ChanAcs;
use Atheme::ReadOnlyHashWrapper;
use Atheme::Internal::HookHash;
use Atheme::Internal::ServiceHash;
use Atheme::Hooks;
use Atheme::Log;

our %Services;
our %Users;
our %Accounts;
our %Channels;
our %ChannelRegistrations;
our %Hooks;

tie %Services, 'Atheme::Internal::ServiceHash';
tie %Users, 'Atheme::ReadOnlyHashWrapper', sub { Atheme::User->find(@_) };
tie %Accounts, 'Atheme::ReadOnlyHashWrapper', sub { Atheme::Account->find(@_) };
tie %Channels, 'Atheme::ReadOnlyHashWrapper', sub { Atheme::Channel->find(@_) };
tie %ChannelRegistrations, 'Atheme::ReadOnlyHashWrapper', sub { Atheme::ChannelRegistration->find(@_) };
tie %Hooks, 'Atheme::Internal::HookHash';

sub depends {
    croak "Use depends entry in \%Info hash instead of depends function\n";
}

1;
