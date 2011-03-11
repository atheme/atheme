package Atheme::ChanAcs;

use constant {
    CA_VOICE        => 0x00000001,
    CA_AUTOVOICE    => 0x00000002,
    CA_OP           => 0x00000004,
    CA_AUTOOP       => 0x00000008,
    CA_TOPIC        => 0x00000010,
    CA_SET          => 0x00000020,
    CA_REMOVE       => 0x00000040,
    CA_INVITE       => 0x00000080,
    CA_RECOVER      => 0x00000100,
    CA_FLAGS        => 0x00000200,
    CA_HALFOP       => 0x00000400,
    CA_AUTOHALFOP   => 0x00000800,
    CA_ACLVIEW      => 0x00001000,
    CA_FOUNDER      => 0x00002000,
    CA_USEPROTECT   => 0x00004000,
    CA_USEOWNER     => 0x00008000,
};

require Exporter;
our @ISA = qw/Exporter/;

our @EXPORT_OK;
our %EXPORT_TAGS = ( all => [ qw(
        CA_VOICE CA_AUTOVOICE CA_OP CA_AUTOOP CA_TOPIC CA_SET CA_REMOVE
        CA_INVITE CA_RECOVER CA_FLAGS CA_HALFOP CA_AUTOHALFOP CA_ACLVIEW
        CA_FOUNDER CA_USEPROTECT CA_USEOWNER
)]);

Exporter::export_ok_tags('all');

1;
