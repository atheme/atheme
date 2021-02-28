## Atheme IRC Services

Atheme is a set of services for IRC networks designed for large IRC networks
with high scalability requirements. It is relatively mature software, with
some code and design derived from another package called Shrike.

Atheme's behavior is tunable using modules and a highly detailed configuration
file. Almost all behavior can be changed at deployment time just by editing
the configuration.



## Obtaining Atheme

If you have a modern version of Git (1.6.5 or newer), you can recursively
clone the repository:

    $ git clone --branch 'release/7.2' --recursive 'https://github.com/atheme/atheme/' atheme-devel
    $ cd atheme-devel

If you have an older version of Git, you must clone the repository, and then
fetch its submodules:

    $ git clone 'https://github.com/atheme/atheme/' atheme-devel
    $ cd atheme-devel
    $ git checkout release/7.2
    $ git submodule init
    $ git submodule update

If you don't have Git, you can download a package archive from our website at
<https://atheme.github.io/>.

If you are browsing our GitHub repository, please do NOT click the "Download
ZIP" button or the "Source code" links there, as they will give you an archive
that lacks the required submodules. There are proper `.tar.bz2` or `.tar.xz`
archives attached to each release under "Assets", which is what the "Download"
button on our website will take you to.



## Basic build instructions for the impatient

Obtain the source code repository and change into its directory (using the
commands given above).

If you are building Atheme on a GNU/Linux system, or something which can
sufficiently emulate that (like WSL 2 on Windows 10), execute the following
commands:

    $ ./configure
    $ make
    $ make install

If you are building Atheme on an OpenBSD (or similar) system, execute the
following commands instead:

    # pkg_add gmake
    $ ./configure
    $ gmake
    $ gmake install



## Getting More Help

If you're still lost, read the [INSTALL](INSTALL) file or check out our wiki
for more hints.

- [Our Website](https://atheme.github.io/)
- [Our GitHub Repository](https://github.com/atheme/atheme/)
- [The Atheme Wiki](https://github.com/atheme/atheme/wiki/)
- [The #atheme channel on freenode](ircs://chat.freenode.net:6697/#atheme)
