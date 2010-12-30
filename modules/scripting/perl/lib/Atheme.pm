package Atheme;

use Exporter 'import';

require DynaLoader;
our @ISA = qw(DynaLoader);

our @EXPORT = qw( %Services &depends );

bootstrap Atheme;

use Atheme::Service;
use Atheme::Services;

our %Services;

tie %Services, 'Atheme::Services';

sub depends {
    Atheme::request_module_dependency $_ foreach @_;
}

1;
