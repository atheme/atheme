package Atheme::Log;

use strict;
use warnings;

use constant {
	admin		=> 0x00000100,
	register	=> 0x00000200,
	set		=> 0x00000400,
	action		=> 0x00000800,
	login		=> 0x00001000,
	get		=> 0x00002000,
	request		=> 0x00004000,
};

1;
