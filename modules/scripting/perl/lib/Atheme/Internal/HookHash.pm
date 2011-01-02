package Atheme::Internal::HookHash;

use strict;
use warnings;

use Carp;

use Atheme::Internal::Hooklist;

require Tie::Hash;

our @ISA = 'Tie::StdHash';

sub TIEHASH {
	my ($class) = @_;
	$class = ref $class || $class;

	return bless {}, $class;
}

sub FETCH {
	my ($self, $hookname) = @_;

	$self->{$hookname} ||= Atheme::Internal::Hooklist->new($hookname);
	return $self->{$hookname};
}

sub readonly {
	croak "Attempted to modify Hooks hash.";
}

sub STORE { goto &readonly; }
sub DELETE { goto &readonly; }
sub CLEAR { goto &readonly; }

1;
