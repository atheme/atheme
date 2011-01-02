package Atheme::Internal::Hooklist;

use strict;
use warnings;

use Atheme::Hooks;

use Carp;

sub new {
	my ($class, $hookname) = @_;
	$class = ref $class || $class;

	croak "Tried to construct a hook list without a name" unless $hookname;

	return bless { name => $hookname, hooks => [] }, $class;
}

sub add_hook {
	my ($self, $hook) = @_;

	my ($caller) = caller;
	$Atheme::Hooks::hooks_by_package{$caller} ||= [];
	push @{$Atheme::Hooks::hooks_by_package{$caller}}, { list => $self, hook => $hook };

	if (scalar @{$self->{hooks}} == 0) {
		enable_perl_hook_handler($self->{name})
	}

	push @{$self->{hooks}}, $hook;
}

sub del_hook {
	my ($self, $hook) = @_;

	my @newhooks;
	foreach my $h (@{$self->{hooks}}) {
		push @newhooks, $h unless $h == $hook;
	}

	if (scalar @newhooks == 0) {
		disable_perl_hook_handler($self->{name});
	}

	$self->{hooks} = \@newhooks;
}

sub call_hooks {
	my ($self, $arg) = @_;

	foreach my $hook (@{$self->{hooks}}) {
		$hook->($arg);
	}
}

1;
