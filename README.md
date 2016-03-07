## atheme

Atheme is a set of services for IRC networks designed for large IRC networks with high
scalability requirements.  It is relatively mature software, with some code and design
derived from another package called Shrike.

Atheme's behavior is tunable using modules and a highly detailed configuration file.
Almost all behavior can be changed at deployment time just by editing the configuration.

If you are running this code from Git, you should read GIT-Access for instructions on
how to fully check out the atheme tree, as it is spread across many repositories.


## basic build instructions for the impatient

Whatever you do, make sure you do *not* install Atheme into the same location as the source.
Atheme will default to installing in `$HOME/atheme`, so make sure you plan accordingly for this.

    $ git submodule update --init
    $ ./configure
    $ make
    $ make install

If you're still lost, read [INSTALL](INSTALL) or [GIT-Access](GIT-Access) for hints. 

## links / contact

 * [GitHub](https://github.com/atheme/atheme)
 * [Website](http://atheme.github.io/)
 * [IRC] irc://chat.freenode.net/ #atheme
