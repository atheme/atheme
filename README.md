## atheme

Atheme is a *legacy* set of services for IRC networks designed for large IRC networks with high
scalability requirements.  It is relatively mature software, with some code and design
derived from another package called Shrike.

Atheme's behavior is tunable using modules and a highly detailed configuration file.
Almost all behavior can be changed at deployment time just by editing the configuration.

If you are running this code from Git, you should read GIT-Access for instructions on
how to fully check out the atheme tree, as it is spread across many repositories.

## discontinuation notice
Due to completion of all defined goals (the development of the IRCv3/IRCv3.1 ecosystem, major
usability changes for services, etc.), the development activity of Atheme is winding down. There
will not be another release cycle after Atheme 7.2. We encourage the community to fork Atheme and
choose the most suitable forks to drive IRC forward. To this end, we will maintain Atheme 7.2 as a
suitable base for forking until October 31, 2015, with all services terminating on October 31,
2016.

To this end, you may find the following table useful:
| Milestone                      | Date            |
| ------------------------------ | --------------- |
| End of non-security bugfixes   | 1 May 2015      |
| End of irc.atheme.org          | 31 October 2015 |
| End of ALL bugfixes            | 1 May 2016      |
| End of ALL atheme.org services | 31 October 2016 |

## basic build instructions for the impatient

Whatever you do, make sure you do *not* install Atheme into the same location as the source.
Atheme will default to installing in `$HOME/atheme`, so make sure you plan accordingly for this.

    $ git submodule update --init
    $ ./configure
    $ make
    $ make install

If you're still lost, read [INSTALL](INSTALL) or [GIT-Access](GIT-Access) for hints.

## links / contact

 * [GitHub](http://www.github.com/atheme/atheme)
 * [Website](http://www.atheme.net)
 * [IRC](irc://irc.atheme.org/#atheme)
