package Atheme::ChanMode;

use strict;
use warnings;

use constant {
    invite      => 1,
    key         => 2,
    limit       => 4,
    mod         => 8,
    noext       => 10,
    priv        => 40,
    sec         => 80
    topic       => 100
    chanreg     => 200
};

require Exporter;
our @ISA = qw/Exporter/;

our @EXPORT_OK;
our %EXPORT_TAGS = ( all => [ qw(
    invite key limit mod noext priv sec topic chanreg
)]);

Exporter::export_ok_tags('all');

1;
