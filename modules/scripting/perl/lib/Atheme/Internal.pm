package Atheme::Internal;

# Bound commands needing to be unbound on script unload.
our %CommandBinds;

sub unbind_commands {
	my ($packagename) = @_;

	if (ref $CommandBinds{$packagename} eq 'ARRAY') {
		foreach my $bind (@{$CommandBinds{$packagename}}) {
			$Atheme::Services{$bind->{service}}->unbind_command($bind->{command});
		}
		delete $CommandBinds{$packagename};
	}
}

1;
