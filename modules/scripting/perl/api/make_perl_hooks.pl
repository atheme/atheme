#!/usr/bin/env perl

use strict;
use warnings;

use FindBin;

my %hooks;
my %arg_types;

# XXX: Types we haven't exposed to perl yet. Remove these if they do become supported.
# THIS LIST IS NOT A SUBSTITUTE FOR ACTUALLY DEFINING HOOK STRUCTURES. IT IS FOR STRUCTURES
# WITH MEMBERS THAT WE CAN'T SUPPORT YET.
my @unsupported_types = ( 'struct database_handle', 'sasl_message_t',
    'hook_module_load_t', 'hook_myentity_req_t', 'hook_host_request_t',
    'hook_channel_acl_req_t', 'hook_email_canonicalize_t', 'struct mygroup' );

# Types that need special handling. Define the dispatch for these, but the handler
# functions themselves are hand-written.
my @special_types = ( 'hook_expiry_req_t' );

# XXX: Duplication here with the typedefs in Atheme.xs.
my %perl_api_types = (
	sourceinfo_t => 'Atheme::Sourceinfo',
	user_t => 'Atheme::User',
	'struct channel' => 'Atheme::Channel',
	'struct chanuser' => 'Atheme::ChanUser',
	server_t => 'Atheme::Server',
	service_t => 'Atheme::Service',
	'struct myuser' => 'Atheme::Account',
	'struct mynick' => 'Atheme::NickRegistration',
	'struct mychan' => 'Atheme::ChannelRegistration',
	'struct chanacs' => 'Atheme::ChanAcs',

	'char *' => [ sub { "sv_setpv($_[0], $_[1]);" }, sub { die "Don't know how to unmarshal a read-write string"; } ],
	'const char *' => [ sub { "sv_setpv($_[0], $_[1]);" }, sub { die "Don't know how to unmarshal a read-write string"; } ],
	'int' => [ sub { "sv_setiv($_[0], $_[1]);" }, sub { "$_[1] = SvIV($_[0]);" } ],
	'time_t' => [ sub { "sv_setiv($_[0], $_[1]);" }, sub { "$_[1]= SvIV($_[0]);" } ],
);

# Define the mapping from C structures to perl hashrefs.
# Each structure is a hash of (member => definition).
# definition can either be a string type (pointers are implicit for supported structure types),
# or an arrayref of [type, name], if the name on the perl side is to be different from the
# member name in the structure. Prefix name with '+' if this value may be modified by the hook.
my %hook_structs = (
	hook_channel_joinpart_t => {
		cu => [ 'struct chanuser', '+chanuser' ]
	},
	hook_cmessage_data_t => {
		u => [ 'user_t', 'user' ],
		c => [ 'struct channel', 'channel' ],
		msg => [ 'char *', 'message' ],
	},
	hook_channel_topic_check_t => {
		u => [ 'user_t', 'user' ],
		s => [ 'server_t', 'server' ],
		c => [ 'struct channel', 'channel' ],
		setter => 'char *',
		ts => 'time_t',
		topic => [ 'char *', 'topic' ],
		approved => [ 'int', '+approved' ],
	},
	hook_channel_req_t => {
		mc => [ 'struct mychan', 'channel' ],
		si => [ 'sourceinfo_t', 'source' ],
	},
	hook_channel_succession_req_t => {
		mc => [ 'struct mychan', 'channel' ],
		mu => [ 'struct myuser', '+account' ],
	},
	hook_channel_register_check_t => {
		si => [ 'sourceinfo_t', 'source' ],
		name => 'const char *',
		chan => [ 'struct channel', 'channel' ],
		approved => [ 'int', '+approved' ],
	},
	hook_user_req_t => {
		si => [ 'sourceinfo_t', 'source' ],
		mu => [ 'struct myuser', 'account' ],
		mn => [ 'struct mynick', 'nick' ],
	},
	hook_user_register_check_t => {
		si => [ 'sourceinfo_t', 'source' ],
		account => 'const char *',
		email => 'const char *',
		password => 'const char *',
		approved => [ 'int', '+approved' ]
	},
	hook_nick_enforce_t => {
		u => [ 'user_t', 'user' ],
		mn => [ 'struct mynick', 'nick' ],
	},
	hook_metadata_change_t => {
		target => 'struct myuser',
		name => 'const char *',
		value => 'char *',
	},
	hook_user_rename_t => {
		mu => [ 'struct myuser', 'account' ],
		oldname => 'const char *',
	},
	hook_server_delete_t => {
		s => [ 'server_t', 'server' ],
	},
	hook_user_nick_t => {
		u => [ 'user_t', '+user' ],
		oldnick => 'const char *',
	},
	hook_channel_mode_t => {
		u => [ 'user_t', 'user' ],
		c => [ 'struct channel', 'channel' ],
	},
	hook_channel_mode_change_t => {
		cu => [ 'struct chanuser', 'chanuser' ],
		mchar => 'int',
		mvalue => 'int',
	},
	hook_user_delete_t => {
		u => [ 'user_t', 'user' ],
		comment => 'const char *',
	},
	hook_sasl_may_impersonate_t => {
		source_mu => [ 'struct myuser', 'source' ],
		target_mu => [ 'struct myuser', 'target' ],
		allowed => [ 'int', '+allowed' ]
	},
	hook_info_noexist_req_t => {
		si => [ 'sourceinfo_t', 'source' ],
		nick => 'const char *'
	},
	hook_user_needforce_t => {
		si => [ 'sourceinfo_t', 'source' ],
		mu => [ 'struct myuser', 'account' ],
		allowed => [ 'int', '+allowed' ]
	},
);

sub c_var_to_sv {
	my ($varname, $typename, $svname) = @_;

	my $type = $perl_api_types{$typename};

	if (ref $type eq 'ARRAY') {
		return $type->[0]->($svname, $varname);
	} else {
		return "$svname = bless_pointer_to_package($varname, \"$type\");";
	}
}

sub c_var_from_sv {
	my ($varname, $typename, $svname) = @_;

	my $type = $perl_api_types{$typename};

	if (ref $type eq 'ARRAY') {
		return $type->[1]->($svname, $varname);
	} else {
		return "if (!SvTRUE($svname)) $varname = NULL;";
	}
}

open my $hooktypes, "$FindBin::Bin/../../../../include/atheme/hooktypes.in"
	or die "Couldn't open hooktypes.in: $!";

while (<$hooktypes>) {
	next if /^#/ or /^$/;
	/^(\w+)\s+(.+)$/;
	my ($hookname, $arg_type) = ($1, $2);
	$arg_type =~ s/ \*$//;
	chomp $arg_type;
	if (!$hookname || !$arg_type) {
		warn "Couldn't parse hooktypes.in line: '$_'\n";
		next;
	}
	next if grep { $_ eq $arg_type } @unsupported_types;
	unless ($perl_api_types{$arg_type} ||
	        $hook_structs{$arg_type} ||
	        (grep { $_ eq $arg_type } @special_types) ||
	        $arg_type eq "void") {
		warn "Hook type '$arg_type' is neither specified for use nor marked as currently unsupported\n";
		next;
	}

	$hooks{$hookname} = $arg_type;
	$arg_types{$arg_type} = 1 unless grep { $_ eq $arg_type } @special_types;
}
close $hooktypes;

open my $outfile, '>', "perl_hooks.c" or die "Couldn't open perl_hooks.h: $!";

print $outfile <<EOF;
/*
 * THIS IS A GENERATED FILE -- DO NOT EDIT.
 *
 * Change and re-run make_perl_hooks.pl instead.
 */
#include "atheme_perl.h"
#include "perl_hooks.h"

typedef enum {
	PERL_HOOK_TO_PERL,
	PERL_HOOK_FROM_PERL
} perl_hook_marshal_direction_t;

#include "perl_hooks_extra.h"

EOF

#
# Write the hook-data marshalling functions.
#

foreach my $arg_type (sort keys %arg_types) {
	next if ($arg_type eq 'void');

	my $arg_type_underscored = $arg_type;
	$arg_type_underscored =~ s/ /_/g;

	if (defined $perl_api_types{$arg_type}) {
		# Simple type. Straightforward marshaller.

		my $conv_code = c_var_to_sv("data", $arg_type, "*psv");

		print $outfile <<"EOF";
static void perl_hook_marshal_$arg_type_underscored (perl_hook_marshal_direction_t dir, $arg_type * data, SV ** psv)
{ if (dir == PERL_HOOK_TO_PERL) $conv_code }

EOF
	} else {
		# Hook struct. Turn it into a hashref.

		my $struct = $hook_structs{$arg_type};

		if (!$struct) {
			warn "Found unknown argument type '$arg_type' in hooktypes.in\n";
			next;
		}

		next if grep { $_ eq $arg_type } @special_types;

		my @members;

		foreach my $member (sort keys %$struct) {
			my $definition = $struct->{$member};
			my ($type, $pretty_name, $rw);

			if (ref $definition eq 'ARRAY') {
				$type = $definition->[0];
				$pretty_name = $definition->[1];
				$rw = 1 if $pretty_name =~ s/^\+//;
			} else {
				$type = $definition;
				$pretty_name = $member;
				$rw = 0;
			}
			push @members, [ $member, $type, $pretty_name, $rw ];
		}

		print $outfile <<"EOF";
static void perl_hook_marshal_$arg_type_underscored (perl_hook_marshal_direction_t dir, $arg_type * data, SV ** psv)
{
	if (dir == PERL_HOOK_TO_PERL)
	{
		HV *hash = newHV();
		SV *sv_tmp = NULL;
EOF

		foreach my $member (@members) {
			my ($name, $type, $pretty_name) = @$member;
			my $namelen = length $pretty_name;
			my $conv_code = c_var_to_sv("data->$name", $type, "sv_tmp");

			print $outfile <<"EOF";
		$conv_code
		hv_store(hash, "$pretty_name", $namelen, sv_tmp, 0);
EOF
		}

		print $outfile <<"EOF";

		*psv = newRV_noinc((SV*)hash);
	} else {
		return_if_fail(SvROK(*psv) && SvTYPE(SvRV(*psv)) == SVt_PVHV);
		HV *hash = (HV*)SvRV(*psv);
		SV **psv_tmp = NULL;
EOF
		foreach my $member (@members) {
			my ($name, $type, $pretty_name, $rw) = @$member;
			my $namelen = length $pretty_name;

			next unless $rw;

			my $convcode = c_var_from_sv("data->$name", $type, "*psv_tmp");

			print $outfile <<"EOF";
		psv_tmp = hv_fetch(hash, "$pretty_name", $namelen, 0);
		$convcode
EOF
		}

		print $outfile <<"EOF";
	}
}

EOF

	}
}

#
# Now write the actual hook handlers.
#

foreach my $hookname (sort keys %hooks) {
	my $arg_type = $hooks{$hookname};
	next if grep { $_ eq $arg_type } @special_types;

	my $arg_type_underscored = $arg_type;
	$arg_type_underscored =~ s/ /_/g;

	print $outfile <<"EOF";
static void perl_hook_$hookname ($arg_type * data)
{
	SV *arg;
	perl_hook_marshal_$arg_type_underscored(PERL_HOOK_TO_PERL, data, &arg);

	dSP;
	ENTER;
	SAVETMPS;
	PUSHMARK(SP);

	XPUSHs(newRV_noinc((SV*)get_cv("Atheme::Hooks::call_hooks", 0)));
	XPUSHs(sv_2mortal(newSVpv("$hookname", 0)));
	XPUSHs(arg);
	PUTBACK;
	call_pv("Atheme::Init::call_wrapper", G_EVAL | G_DISCARD);

	SPAGAIN;

	if (SvTRUE(ERRSV))
	{
		slog(LG_ERROR, "Calling perl hook $hookname raised unexpected error %s", SvPV_nolen(ERRSV));
	}

	FREETMPS;
	LEAVE;

	perl_hook_marshal_$arg_type_underscored(PERL_HOOK_FROM_PERL, data, &arg);
	SvREFCNT_dec(arg);
	invalidate_object_references();
}

EOF

}

# Now, the functions that Perl will call to enable/disable a hook handler.

print $outfile <<EOF;

void enable_perl_hook_handler(const char *hookname)
{
EOF
foreach my $hookname (sort keys %hooks) {
	print $outfile <<"EOF";
	if (0 == strcmp(hookname, "$hookname")) {
		hook_add_$hookname(perl_hook_$hookname);
		return;
	}
EOF
}
print $outfile <<EOF;

	dTHX;
	Perl_croak(aTHX_ "Attempted to enable unknown hook name %s", hookname);
}

void disable_perl_hook_handler(const char *hookname)
{
EOF
foreach my $hookname (sort keys %hooks) {
	print $outfile <<"EOF";
	if (0 == strcmp(hookname, "$hookname")) {
		hook_del_$hookname(perl_hook_$hookname);
		return;
	}
EOF
}
print $outfile <<EOF;

	dTHX;
	Perl_croak(aTHX_ "Attempted to disable unknown hook name %s", hookname);
}

EOF

close $outfile;
