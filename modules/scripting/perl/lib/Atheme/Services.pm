package Atheme::Services;

use Carp;

sub TIEHASH {
	my ($class) = @_;
	$class = ref $class || $class;

	my $self = {};

	return bless $self, $class;
}

sub FETCH {
	my ($self, $key) = @_;

	return Atheme::Service->find($key);
}

sub readonly {
	croak "Attempted to modify read-only Services hash\n";
}

sub STORE { goto &readonly; }
sub DELETE { goto &readonly; }
sub CLEAR { goto &readonly; }

sub EXISTS {
	my ($self, $key) = @_;

	return defined $self->FETCH($key);
}

sub unimplemented {
	croak "Iteration over Services hash not yet implemented\n";
}

sub FIRSTKEY { goto &unimplemented; }
sub NEXTKEY { goto &unimplemented; }

sub SCALAR {
	return 1; # Generic true value. There's always at least one service.
}

1;
