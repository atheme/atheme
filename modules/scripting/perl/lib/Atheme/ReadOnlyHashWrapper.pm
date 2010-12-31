package Atheme::ReadOnlyHashWrapper;

use Carp;

sub TIEHASH {
	my ($class, $find_func) = @_;
	$class = ref $class || $class;

	my $self = { find_func => $find_func };

	return bless $self, $class;
}

sub FETCH {
	my ($self, $key) = @_;

	return $self->{find_func}->($key);
}

sub readonly {
	croak "Attempted to modify read-only hash\n";
}

sub STORE { goto &readonly; }
sub DELETE { goto &readonly; }
sub CLEAR { goto &readonly; }

sub EXISTS {
	my ($self, $key) = @_;

	return defined $self->FETCH($key);
}

sub unimplemented {
	croak "Iteration over Atheme object hash not yet implemented\n";
}

sub FIRSTKEY { goto &unimplemented; }
sub NEXTKEY { goto &unimplemented; }

sub SCALAR {
	return 1; # Generic true value. There's always at least one client.
}

1;
