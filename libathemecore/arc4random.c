/*
 * Arc4 random number generator for OpenBSD.
 * Copyright 1996 David Mazieres <dm@lcs.mit.edu>.
 *
 * Modification and redistribution in source and binary forms is
 * permitted provided that due credit is given to the author and the
 * OpenBSD project (for instance by leaving this copyright notice
 * intact).
 */

/*
 * This code is derived from section 17.1 of Applied Cryptography,
 * second edition, which describes a stream cipher allegedly
 * compatible with RSA Labs "RC4" cipher (the actual description of
 * which is a trade secret).  The same algorithm is used as a stream
 * cipher called "arcfour" in Tatu Ylonen's ssh package.
 *
 * Here the stream cipher has been modified always to include the time
 * when initializing the state.  That makes it impossible to
 * regenerate the same random sequence twice, so this can't be used
 * for encryption, but will generate good random numbers.
 *
 * RC4 is a registered trademark of RSA Laboratories.
 */

/*
 * $FreeBSD: /usr/local/www/cvsroot/FreeBSD/src/lib/libc/gen/arc4random.c,v 1.10.8.1 2006/11/18 21:35:13 ache Exp $
 * Modified for Atheme by Jilles Tjoelker
 */

#include "atheme.h"

#ifndef HAVE_ARC4RANDOM

struct arc4_stream {
	unsigned char i;
	unsigned char j;
	unsigned char s[256];
};

#define	RANDOMDEV	"/dev/urandom"

static struct arc4_stream rs;
static int rs_initialized;
static int rs_stired;
static int arc4_count;

static inline unsigned char arc4_getbyte(struct arc4_stream *);
static void arc4_stir(struct arc4_stream *);

static inline void
arc4_init(struct arc4_stream *as)
{
	int     n;

	for (n = 0; n < 256; n++)
		as->s[n] = n;
	as->i = 0;
	as->j = 0;
}

static inline void
arc4_addrandom(struct arc4_stream *as, unsigned char *dat, int datlen)
{
	int     n;
	unsigned char si;

	as->i--;
	for (n = 0; n < 256; n++) {
		as->i = (as->i + 1);
		si = as->s[as->i];
		as->j = (as->j + si + dat[n % datlen]);
		as->s[as->i] = as->s[as->j];
		as->s[as->j] = si;
	}
}

static void
arc4_stir(struct arc4_stream *as)
{
	int     fd, n;
	struct {
		struct timeval tv;
		pid_t pid;
		unsigned char rnd[128 - sizeof(struct timeval) - sizeof(pid_t)];
	}       rdat;

	gettimeofday(&rdat.tv, NULL);
	rdat.pid = getpid();
	fd = open(RANDOMDEV, O_RDONLY, 0);
	if (fd >= 0) {
		n = read(fd, rdat.rnd, sizeof(rdat.rnd));
		close(fd);
	} 
	/* fd < 0?  Ah, what the heck. We'll just take whatever was on the
	 * stack... */

	arc4_addrandom(as, (void *) &rdat, sizeof(rdat));

	/*
	 * Throw away the first N bytes of output, as suggested in the
	 * paper "Weaknesses in the Key Scheduling Algorithm of RC4"
	 * by Fluher, Mantin, and Shamir.  N=1024 is based on
	 * suggestions in the paper "(Not So) Random Shuffles of RC4"
	 * by Ilya Mironov.
	 */
	for (n = 0; n < 1024; n++)
		(void) arc4_getbyte(as);
	arc4_count = 400000;
}

static inline unsigned char
arc4_getbyte(struct arc4_stream *as)
{
	unsigned char si, sj;

	as->i = (as->i + 1);
	si = as->s[as->i];
	as->j = (as->j + si);
	sj = as->s[as->j];
	as->s[as->i] = sj;
	as->s[as->j] = si;

	return (as->s[(si + sj) & 0xff]);
}

static inline unsigned int
arc4_getword(struct arc4_stream *as)
{
	unsigned int val;

	val = arc4_getbyte(as) << 24;
	val |= arc4_getbyte(as) << 16;
	val |= arc4_getbyte(as) << 8;
	val |= arc4_getbyte(as);

	return (val);
}

static void
arc4_check_init(void)
{
	if (rs_initialized)
		return;

	arc4_init(&rs);
	rs_initialized = 1;
}

static void
arc4_check_stir(void)
{
	if (!rs_stired || --arc4_count == 0) {
		arc4_stir(&rs);
		rs_stired = 1;
	}
}

void
arc4random_stir(void)
{
	arc4_check_init();
	arc4_stir(&rs);
}

void
arc4random_addrandom(unsigned char *dat, int datlen)
{
	arc4_check_init();
	arc4_check_stir();
	arc4_addrandom(&rs, dat, datlen);
}

unsigned int
arc4random(void)
{
	unsigned int rnd;

	arc4_check_init();
	arc4_check_stir();
	rnd = arc4_getword(&rs);

	return (rnd);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */

#endif /* !HAVE_ARC4RANDOM */
