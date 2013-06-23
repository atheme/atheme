package Atheme::ChanMode;

use strict;
use warnings;

use constant {
    invite      => 0x00000001,
    key         => 0x00000002,
    limit       => 0x00000004,
    mod         => 0x00000008,
    noext       => 0x00000010,
    priv        => 0x00000040,
    sec         => 0x00000080,
    topic       => 0x00000100,
    chanreg     => 0x00000200
};

require Exporter;
our @ISA = qw/Exporter/;

our @EXPORT_OK;
our %EXPORT_TAGS = ( all => [ qw(
    invite key limit mod noext priv sec topic chanreg
)]);

Exporter::export_ok_tags('all');

1;
