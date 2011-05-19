package Atheme::Internal::ServiceHash;

use strict;
use warnings;

use Atheme::Service;
use Atheme::Log;

use Carp;

require Tie::Hash;

our @ISA = 'Tie::StdHash';

sub TIEHASH {
	my ($class) = @_;
	$class = ref $class || $class;

	return bless {}, $class;
}

sub FETCH {
	my ($self, $service) = @_;

	Atheme::Log->debug("perl -- looking up $service");

	$self->{$service} ||= Atheme::Service->find($service) or Atheme::Service->create($service);
	return $self->{$service};
}

sub readonly {
	croak "Attempted to modify Service hash.";
}

sub STORE { goto &readonly; }
sub DELETE { goto &readonly; }
sub CLEAR { goto &readonly; }

1;
