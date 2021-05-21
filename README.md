## Atheme IRC Services

Atheme is a set of IRC services designed for large IRC networks with high
scalability requirements. It is relatively mature software, with some code
and design derived from another package called Shrike.

Atheme's behavior is tunable using modules and a highly detailed
configuration file. Almost all behavior can be changed at deployment time
just by editing the configuration.



## Obtaining Atheme

If you have a modern version of Git (1.6.5 or newer), you can recursively
clone the repository:

    $ git clone --recursive 'https://github.com/atheme/atheme/' atheme-devel
    $ cd atheme-devel

If you have an older version of Git, you must clone the repository, and then
fetch its submodules:

    $ git clone 'https://github.com/atheme/atheme/' atheme-devel
    $ cd atheme-devel
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
    $ ./configure --disable-linker-defs
    $ gmake
    $ gmake install



## Library Detection

If your user-installed libraries that you want Atheme to use are installed by
your package manager to a directory such as `/usr/local/`, you may need to
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
pkg-config discovery for those libraries. For example, if you wish to use GNU
libidn, and it is installed into a default search path for your compiler and
linker, and you do not have pkg-config installed, execute:

    $ ./configure LIBIDN_CFLAGS="" LIBIDN_LIBS="-lidn"

If a library relies on populating `LIBFOO_CFLAGS` with some preprocessor
definitions, or populating `LIBFOO_LIBS` with some library linking flags,
this will generally fail. Install pkg-config for the best results.



## Choice of compiler and its features

If you wish to compile Atheme with the LLVM project's C compiler (`clang`),
you may also wish to use LLVM's linker (`lld`). You can accomplish this as
follows:

    $ ./configure CC="clang" LDFLAGS="-fuse-ld=lld"

If you want to use compiler sanitizers, and you want to build with Clang, you
MUST also use LLD, as most of the sanitizers in Clang require LTO to function
properly, and Clang in LTO mode emits LLVM bitcode, not machine code. The
linker is ultimately responsible for performing most of the LTO heavy lifting,
and translating the result into machine code, and most other linkers do not
know how to do this.

To use compiler sanitizers with GCC (supported):

    $ ./configure --disable-heap-allocator --disable-linker-defs \
        --enable-compiler-sanitizers CC="gcc"

To use compiler sanitizers with Clang (recommended):

    $ ./configure --disable-heap-allocator --disable-linker-defs \
        --enable-compiler-sanitizers CC="clang" LDFLAGS="-fuse-ld=lld"

If you do enable the sanitizers, it is recommended to enable the configuration
option `general::db_save_blocking`; see the example configuration file for
more details.

The sanitizers are not recommended for production usage, but they are
recommended for developers, including third parties writing new features
and/or modifying the source code.



## Getting More Help

If you're still lost, read the [INSTALL](INSTALL) file or check out our wiki
for more hints.

- [Our Website](https://atheme.github.io/)
- [Our GitHub Repository](https://github.com/atheme/atheme/)
- [The Atheme Wiki](https://github.com/atheme/atheme/wiki/)
- [The #atheme channel on Libera Chat](ircs://irc.libera.chat:6697/#atheme)
