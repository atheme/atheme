package Atheme::Hooks;

use strict;
use warnings;

our %hooks_by_package;

sub call_hooks {
	my ($hookname, $arg) = @_;

	my $hooklist = $Atheme::Hooks{$hookname};
	$hooklist->call_hooks($arg);
}

sub unbind_hooks {
	my ($packagename) = @_;

	my $hooklist = $hooks_by_package{$packagename};

	foreach my $hook (@$hooklist) {
		$hook->{list}->del_hook($hook->{hook});
	}

	delete $hooks_by_package{$packagename};
}

1;
