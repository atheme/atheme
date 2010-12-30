package Atheme::Command;

sub new {
	my ($class, $args) = @_;

	die "Can't create a command with no name" unless $args->{name};
	die "Can't create a command with no description" unless $args->{desc};
	$args->{maxparc} ||= 1;
	die "Can't create a command with no handler" unless $args->{handler};

	return Atheme::Command->create(
		$args->{name},
		$args->{desc},
		$args->{access},
		$args->{maxparc},
		$args->{help_path},
		$args->{help_func},
		$args->{handler}
	);
}

1;
