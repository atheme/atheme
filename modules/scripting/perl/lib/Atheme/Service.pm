package Atheme::Service;

use Atheme::Command;
use Atheme::Internal;

sub bind_command {
	my ($self, %args) = @_;

	my $command = Atheme::Command->new(\%args);

	my ($caller) = caller;

	$Atheme::Internal::CommandBinds{$caller} ||= [];
	push @{$Atheme::Internal::CommandBinds{$caller}}, { service => $self, command => $command };

	$self->do_bind_command($command);
}

1;
