# ircd_catserv perl edition

use Atheme;
use Atheme::Fault qw/:all/;

%Info = (
	name => 'catserv.pl',
);

Atheme::Log->info ("CatServ is " . $Services{catserv});

# xxx: this has to come second, as we have to lookup the object once in order
# to dereference, maybe spb has a solution. --nenolod
$Services{catserv}->bind_command(
	name => "MEOW",
	desc => "Makes the cute little kitty-cat meow!",
	help_path => "contrib/catserv/meow",
	handler => \&cmd_hello
);

sub cmd_hello {
	my ($source, @parv) = @_;

	$source->success("Meow!");
}
