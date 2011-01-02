package Atheme::Fault;

use strict;
use warnings;

use constant {
	needmoreparams	=> 1,
	badparams	=> 2,
	nosuch_source	=> 3,
	nosuch_target	=> 4,
	authfail	=> 5,
	noprivs		=> 6,
	nosuch_key	=> 7,
	alreadyexists	=> 8,
	toomany		=> 9,
	emailfail	=> 10,
	notverified	=> 11,
	nochange	=> 12,
	already_authed	=> 13,
	unimplemented	=> 14,
	badauthcookie	=> 15
};

require Exporter;
our @ISA = qw/Exporter/;

our @EXPORT_OK;
our %EXPORT_TAGS = ( all => [ qw(
	needmoreparams badparams nosuch_source nosuch_target authfail noprivs nosuch_key alreadyexists
	toomany emailfail notverified nochange already_authed unimplemented badauthcookie
)]);

Exporter::export_ok_tags('all');

1;
