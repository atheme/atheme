package Atheme::Init;

=head1 init.pl -- perl initialisation for Atheme.

This file is processed on initialising the perl interpreter in C<perl.so>. It is
mainly concerned with setting up the Perl environment, maintaining the list of
loaded perl scripts, and providing the means to load and unload them.

=cut

use strict;
use warnings;

use FindBin;
use Symbol qw(delete_package);

use lib "$FindBin::Bin";

use Atheme;
use Atheme::Internal;

our %Scripts;

sub package_name_ify {
    my ($filename) = @_;

    $filename =~ s/([^A-Za-z0-9\/])/sprintf("_%2x",unpack("C",$1))/eg;
    # second pass only for words starting with a digit
    $filename =~ s|/(\d)|sprintf("/_%2x",unpack("C",$1))|eg;

    # Dress it up as a real package name
    $filename =~ s|/|::|g;
    return "Embed" . $filename;
}

sub load_script {
    my ($filename, $message, $nonfatal) = @_;
    my $packagename = package_name_ify($filename);

    if (defined $Scripts{$packagename})
    {
        if ($nonfatal) {
            $message->reply("$filename is already loaded");
            return;
        }

        # This will always be called with G_EVAL, so this die can be translated
        # into a load error in the C module.
        die "$filename is already loaded.";
    }

    local *FH;
    open FH, $filename or die "Couldn't open $filename: $!";
    local ($/) = undef;
    my $script_text = <FH>;
    close FH;

    # Must all be on one line here to avoid messing up line numbers in error
    # messages.
    my $eval = "package $packagename; our %Info; $script_text";
    {
        my ($filename, $packagename, $script_text);
        eval $eval;
    }
    if ($@) {
        delete_package $packagename;
        die $@;
    }

    $Scripts{$packagename} = $filename;
    return $packagename;
}

sub unload_script {
    my ($filename, $message) = @_;
    my $packagename = package_name_ify($filename);

    if (!defined $Scripts{$packagename})
    {
        die "$filename is not loaded";
    }

    eval "${packagename}::unload(\$message)";

    Atheme::Internal::unbind_commands($packagename);
    Atheme::Hooks::unbind_hooks($packagename);

    delete_package($packagename);
    delete $Scripts{$packagename};
}

sub list_scripts {
    my ($si) = @_;
    $si->success("Loaded scripts:");
    my $num = 0;
    foreach my $script (values %Scripts) {
            $num++;
            $si->success("$num: $script");
    }
    $si->success("\x02$num\x02 scripts loaded.");
}

sub call_wrapper {
    my $sub = shift;
    my $saved_alarm=$SIG{ALRM};
    $SIG{ALRM} = sub { die "Script used too much running time"; };
    my $ret;
    eval {
            alarm 1;
            $ret = &$sub(@_);
    };
    alarm 0;
    $SIG{ALRM} = $saved_alarm;
    die $@ if $@;
    return $ret;
}

