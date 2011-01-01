package Atheme;

use Exporter 'import';

require DynaLoader;
our @ISA = qw(DynaLoader);

our @EXPORT = qw( %Services %Users %ChannelRegistrations &depends );

bootstrap Atheme;

use Atheme::Service;
use Atheme::Account;
use Atheme::ChannelRegistration;
use Atheme::ReadOnlyHashWrapper;

our %Services;
our %Users;
our %Channels;
our %ChannelRegistrations;

tie %Services, 'Atheme::ReadOnlyHashWrapper', sub { Atheme::Service->find(@_) };
tie %Users, 'Atheme::ReadOnlyHashWrapper', sub { Atheme::User->find(@_) };
tie %Channels, 'Atheme::ReadOnlyHashWrapper', sub { Atheme::Channel->find(@_) };
tie %ChannelRegistrations, 'Atheme::ReadOnlyHashWrapper', sub { Atheme::ChannelRegistration->find(@_) };

sub depends {
    Atheme::request_module_dependency $_ foreach @_;
}

1;
