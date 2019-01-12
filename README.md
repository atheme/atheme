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

If you're still lost, read the [INSTALL](INSTALL) or
[GIT-Access.txt](GIT-Access.txt) files or check out our wiki
(http://github.com/atheme/atheme/wiki) for more hints.

## links / contact

 * [GitHub](https://github.com/atheme/atheme)
 * [Website](http://atheme.github.io/)
 * [Wiki](https://github.com/atheme/atheme/wiki)
 * [Freenode / #atheme](ircs://chat.freenode.net:6697/#atheme)
