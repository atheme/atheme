package Atheme;

use Exporter 'import';

require DynaLoader;
our @ISA = qw(DynaLoader);

our @EXPORT = qw( %Users %Services &depends );

bootstrap Atheme;

use Atheme::Service;
use Atheme::ReadOnlyHashWrapper;

our %Services;
our %Users;

tie %Services, 'Atheme::ReadOnlyHashWrapper', sub { Atheme::Service->find(@_) };
tie %Users, 'Atheme::ReadOnlyHashWrapper', sub { Atheme::User->find(@_) };

sub depends {
    Atheme::request_module_dependency $_ foreach @_;
}

1;
