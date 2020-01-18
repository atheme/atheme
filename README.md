## Atheme IRC Services

Atheme is a set of IRC services designed for large IRC networks with high
scalability requirements. It is relatively mature software, with some code
and design derived from another package called Shrike.

Atheme's behavior is tunable using modules and a highly detailed
configuration file. Almost all behavior can be changed at deployment time
just by editing the configuration.


## Obtaining Atheme
You can either git clone https://github.com/atheme/atheme.git or download a
package via our website at https://atheme.github.io/ -- Please do not click
the Download buttons on GitHub as they lack needed submodules, etc.

If you are running this code from Git, you should read GIT-Access.txt for
instructions on how to fully check out the atheme tree, as it is spread
across many repositories.


## Basic build instructions for the impatient

Whatever you do, make sure you do *not* install Atheme into the same location
as the source. Atheme will default to installing in `$HOME/atheme`, so make
sure you plan accordingly for this.

    $ git submodule update --init
    $ ./configure
    $ make
    $ make install

If you are on an OpenBSD system, or similar, you will need to do things
slightly differently:

    # pkg_add gmake
    $ git submodule update --init
    $ ./configure --disable-linker-defs
    $ gmake
    $ gmake install

If your user-installed libraries that you want Atheme to use are installed by
your package manager to a directory such as /usr/local/, you may need to
supplement the default compiler and linker search paths so that Atheme can
detect those libraries (e.g. cracklib from FreeBSD Ports):

    $ ./configure CPPFLAGS="-I/usr/local/include" LDFLAGS="-L/usr/local/lib"

The following libraries generally require pkg-config to be installed:

- PHC Argon2 Reference Implementation (`libargon2.pc`)
- OpenSSL (`libcrypto.pc`)
- GNU libidn (`libidn.pc`)
- GNU Nettle (`nettle.pc`)
- PCRE (`libpcre.pc`)
- libqrencode (`libqrencode.pc`)
- Sodium (`libsodium.pc`)

If you do not have pkg-config installed and want to use one or more of these
libraries, please see `./configure --help` for the options to set to override
pkg-config discovery for those libraries. For example, if you wish to use
GNU libidn, and it is installed into a default search path for your compiler
and linker, and you do not have pkg-config installed, execute:

    $ ./configure LIBIDN_CFLAGS="" LIBIDN_LIBS="-lidn"

If a library relies on populating `LIBFOO_CFLAGS` with some preprocessor
definitions, or populating `LIBFOO_LIBS` with some library linking flags,
this will generally fail. Install pkg-config for the best results.

If you're still lost, read the [INSTALL](INSTALL) or
[GIT-Access.txt](GIT-Access.txt) files or check out our wiki
(http://github.com/atheme/atheme/wiki) for more hints.

## links / contact

 * [GitHub](https://github.com/atheme/atheme)
 * [Website](http://atheme.github.io/)
 * [Wiki](https://github.com/atheme/atheme/wiki)
 * [Freenode / #atheme](ircs://chat.freenode.net:6697/#atheme)
