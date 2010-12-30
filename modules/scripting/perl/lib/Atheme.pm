package Atheme;

use Exporter 'import';

require DynaLoader;
our @ISA = qw(DynaLoader);

our @EXPORT = qw( %Services );
our @EXPORT_OK = qw( %Services );

bootstrap Atheme;

use Atheme::Service;
use Atheme::Services;

our %Services;

tie %Services, 'Atheme::Services';

1;
