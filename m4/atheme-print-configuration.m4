AC_DEFUN([ATHEME_PRINT_CONFIGURATION], [

	AS_IF([test "x${ARC4RANDOM_BUILDING}" = "xYes"], [ARC4RANDOM_BUILDING="Yes (Internal ChaCha20)"])

	prefix="$(eval echo "${prefix}")"
	prefix="$(eval echo "${prefix}")"

	bindir="$(eval echo "${bindir}")"
	bindir="$(eval echo "${bindir}")"

	MODDIR="$(eval echo "${MODDIR}")"
	MODDIR="$(eval echo "${MODDIR}")"

	sysconfdir="$(eval echo "${sysconfdir}")"
	sysconfdir="$(eval echo "${sysconfdir}")"

	LOGDIR="$(eval echo "${LOGDIR}")"
	LOGDIR="$(eval echo "${LOGDIR}")"

	DATADIR="$(eval echo "${DATADIR}")"
	DATADIR="$(eval echo "${DATADIR}")"

	RUNDIR="$(eval echo "${RUNDIR}")"
	RUNDIR="$(eval echo "${RUNDIR}")"

	echo "
Configuration:

    Atheme Version ..........: ${PACKAGE_VERSION}

    Installation Prefix .....: ${prefix}
    Binary Directory ........: ${bindir}
    Module Directory ........: ${MODDIR}/modules
    Config Directory ........: ${sysconfdir}
    Logfile Directory .......: ${LOGDIR}
    Data Directory ..........: ${DATADIR}
    PID Directory ...........: ${RUNDIR}

    Contrib Modules .........: ${CONTRIB_MODULES}
    Internationalization ....: ${USE_GETTEXT}
    Large Network Support ...: ${LARGE_NET}
    Reproducible Builds .....: ${REPRODUCIBLE_BUILDS}
    Build Warnings ..........: ${BUILD_WARNINGS}

    crypt(3) Support ........: ${LIBCRYPT}
    CrackLib Support ........: ${LIBCRACKLIB}
    LDAP Support ............: ${LIBLDAP}
    GNU libidn Support ......: ${LIBIDN}
    ARM mbedTLS Support .....: ${LIBMBEDCRYPTO}
    Nettle Support ..........: ${LIBNETTLE}
    OpenSSL Support .........: ${LIBCRYPTO}
    PCRE Support ............: ${LIBPCRE}
    Perl Support ............: ${LIBPERL}
    QR Code Support .........: ${LIBQRENCODE}

    Digest Frontend .........: ${DIGEST_FRONTEND}
    Mowgli Installation .....: ${MOWGLI_SOURCE}

    Fallback RNG Enabled ....: ${ARC4RANDOM_BUILDING}

    CC ......................: ${CC}
    CFLAGS ..................: ${CFLAGS}
    CPPFLAGS ................: ${CPPFLAGS}
    LDFLAGS .................: ${LDFLAGS}
    LIBS ....................: ${LIBS}

Type make to build Atheme, and make install to install it.
"

])
