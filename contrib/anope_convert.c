/*
 * This is an ANOPE module to convert anope databases to atheme.
 * Compile and load this (you may need to do some tricks; hint: if
 * it won't load the first time try another time), and an atheme
 * flatfile db will be dumped to WHERE_TO.
 *
 * Caveats:
 * 1. Most likely, only channels with xOP access are converted correctly.
 * 2. Unknown access levels except -1, -2 are converted to AOP.
 *
 * Errors will appear in the converted atheme.db as comments, for example:
 * # Could not convert #chat::nenolod (level 999), out of bound
 *
 * Good luck, and have fun!
 */

#define WHERE_TO "/home/jilles/atheme.db"

/* define this if you use an anope that has everything in root, i.e. "pre CAPAB", 1.7.4 or lower. */
/* anope 1.7.x before 1.7.13 will probably not work */
#define ANOPE_16X

#ifndef ANOPE_16X
#include "services.h"
#include "pseudo.h"
#else
#include "../services.h"
#include "../pseudo.h"
#endif

/* define this if you want to convert forbids */
#undef CONVERT_FORBID

/* define this if you want to keep encrypted passwords as such
 * (this requires using the ircservices crypto module) */
#define CONVERT_CRYPTPASS

extern NickAlias *nalists[1024];
extern ChannelInfo *chanlists[256];
extern SList akills;
int muout = 0, mcout = 0, caout = 0, klnout = 0;
FILE *f;

void write_accounts(void)
{
	int i, ii;
	NickAlias *na;
	int athemeflags;
	char passwdbuf[33];
	char *passwd;

	if (!f)
		return;

	for (i = 0; i < 1024; i++) {
		for (na = nalists[i]; na; na = na->next) {
			if (na->status & NS_VERBOTEN)
				continue;

			athemeflags = 0;
			if (na->status & NS_NO_EXPIRE)
				athemeflags |= 0x1; /* MU_HOLD */
			if (na->nc->flags & NI_HIDE_EMAIL)
				athemeflags |= 0x10; /* MU_HIDEMAIL */
			if (na->nc->flags & NI_ENCRYPTEDPW)
			{
#ifdef CONVERT_CRYPTPASS
				athemeflags |= 0x100; /* MU_CRYPTPASS */
				/* this stuff may contain '\0' and other
				 * garbage, encode it -- jilles */
				strcpy(passwdbuf, "$ircservices$");
				/* the encrypted password is 16 bytes long,
				 * but the last 8 bytes are constant,
				 * omit them */
				for (ii = 0; ii <= 7; ii++)
					sprintf(passwdbuf + 13 + 2 * ii, "%02x",
							255 & (int)na->nc->pass[ii]);
				passwd = passwdbuf;
#else
				passwd = na->nick;
#endif
			}
			else
				passwd = na->nc->pass;
			if (na->nc->memos.memomax == 0)
				athemeflags |= 0x40; /* MU_NOMEMO */
			fprintf(f, "MU %s %s %s %lu %lu 0 0 0 %d\n", na->nick,
					passwd, na->nc->email, (unsigned long)na->time_registered,
					(unsigned long)na->last_seen, athemeflags);
			if (na->last_usermask != NULL)
			{
				/*fprintf(f, "MD U %s private:host:actual %s", na->nick, na->last_usermask);*/
				fprintf(f, "MD U %s private:host:vhost %s\n", na->nick, na->last_usermask);
			}
			if (na->nc->greet)
				fprintf(f, "MD U %s greet %s\n", na->nick, na->nc->greet);
			if (na->nc->icq)
				fprintf(f, "MD U %s icq %u\n", na->nick, (unsigned int)na->nc->icq);
			if (na->nc->url)
				fprintf(f, "MD U %s url %s\n", na->nick, na->nc->url);
						
			muout++;
		}
	}
}

#ifdef ANOPE_16X
static int
convert_mlock(int modes)
{
	int amodes = 0;

	if (modes & CMODE_i)        amodes |= 0x1;
	if (modes & CMODE_m)        amodes |= 0x8;
	if (modes & CMODE_n)        amodes |= 0x10;
	if (modes & CMODE_p)        amodes |= 0x40;
	if (modes & CMODE_s)        amodes |= 0x80;
	if (modes & CMODE_t)        amodes |= 0x100;
	if (modes & CMODE_k)        amodes |= 0x2;
	if (modes & CMODE_l)        amodes |= 0x4;
	/* the others differ per ircd in atheme :( -- jilles */
	return amodes;
}
#else
static int
convert_mlock(int modes)
{
	int amodes = 0;
	int i;

	for (i = 0; cbmodeinfos[i].mode != '\0'; i++)
	{
		if (!(modes & cbmodeinfos[i].flag))
			continue;
		switch (cbmodeinfos[i].mode)
		{
			case 'i': amodes |= 0x1; break;
			case 'm': amodes |= 0x8; break;
			case 'n': amodes |= 0x10; break;
			case 'p': amodes |= 0x40; break;
			case 's': amodes |= 0x80; break;
			case 't': amodes |= 0x100; break;
			case 'k': amodes |= 0x2; break;
			case 'l': amodes |= 0x4; break;
			/* the others differ per ircd in atheme :( -- jilles */
		}
	}
	return amodes;
}
#endif

void write_channels(void)
{
	int i, j;
	ChannelInfo *ci;
	int athemeflags;
	int athememon, athememoff;
	char *flags;

	if (!f)
		return;

	for (i = 0; i < 256; i++) {
		for (ci = chanlists[i]; ci; ci = ci->next) {
			athemeflags = 0;
			if (ci->flags & CI_NO_EXPIRE)
				athemeflags |= 1; /* MC_HOLD */
			if (ci->flags & CI_SECUREOPS)
				athemeflags |= 8; /* MC_SECURE */
			if (ci->flags & CI_KEEPTOPIC)
				athemeflags |= 0x40; /* MC_KEEPTOPIC */
			if (ci->flags & CI_OPNOTICE)
				athemeflags |= 0x10; /* MC_VERBOSE (not equiv) */
			/* don't include CMODE_KEY/CMODE_LIMIT in mlock_on */
			athememon = convert_mlock(ci->mlock_on) & ~6;
			athememoff = convert_mlock(ci->mlock_off);

#ifndef CONVERT_FORBIDS
			if (ci->flags & CI_VERBOTEN)
				continue;
#endif

			fprintf(f, "MC %s 0 %s %lu %lu %d %d %d %d %s\n", ci->name, 
				(ci->flags & CI_VERBOTEN) ? 
				ci->forbidby : ci->founder->display, 
				(unsigned long)ci->time_registered,
				(unsigned long)ci->last_used, athemeflags,
				athememon, athememoff,
				ci->mlock_limit ? ci->mlock_limit : 0, 
				ci->mlock_key ? ci->mlock_key : "");

			mcout++;
			fprintf(f, "CA %s %s %s\n", ci->name, ci->founder->display, "+AvhoOtsirRf");
			caout++;

			for (j = 0; j < ci->accesscount; j++) {
				if (!ci->access[j].in_use)
					continue;
				/* duplicate founder entry? */
				if (ci->access[j].nc == ci->founder)
					continue;
				flags = NULL;
				/* These do not match atheme's xOP levels,
				 * however they represent the privileges
				 * better -- jilles */
				switch (ci->access[j].level)
				{
					case ACCESS_VOP: flags = "+AV"; break;
					case ACCESS_HOP: flags = "+AvVHt"; break;
					case ACCESS_AOP: flags = "+AvhoOtir"; break;
					case ACCESS_SOP: flags = "+AvhoOtsirf"; break;
					case -1: case -2: break;
					default:
						fprintf(f, "# Access Entry %s::%s level %d unknown, using +AvhoOtir\n",
							ci->name, ci->access[j].nc->display, ci->access[j].level);
						/* most channels don't use any lower access than AOP anyway */
						flags = "+AvhoOtir";
				}
				if (flags != NULL)
				{
					fprintf(f, "CA %s %s %s\n", ci->name,
						ci->access[j].nc->display, flags);
					caout++;
				}
			}

			for (j = 0; j < ci->akickcount; j++) {
				if (!ci->akick[j].in_use)
					continue;
				fprintf(f, "CA %s %s +b\n", ci->name,
					(ci->akick[j].flags & AK_ISNICK) ? ci->akick[j].u.nc->display :
					ci->akick[j].u.mask);
				caout++;
			}

			if (ci->url)
				fprintf(f, "MD C %s url %s\n", ci->name, ci->url);

			if (ci->email)
				fprintf(f, "MD C %s email %s\n", ci->name, ci->email);

			if (ci->desc)
				fprintf(f, "MD C %s description %s\n", ci->name, ci->desc);

			if (ci->entry_message)
				fprintf(f, "MD C %s private:entrymsg %s\n", ci->name, ci->entry_message);

			if (ci->last_topic && ci->last_topic_setter[0] &&
					ci->last_topic_time)
			{
				fprintf(f, "MD C %s private:topic:text %s\n",
						ci->name, ci->last_topic);
				fprintf(f, "MD C %s private:topic:setter %s\n",
						ci->name, ci->last_topic_setter);
				fprintf(f, "MD C %s private:topic:ts %lu\n",
						ci->name, (unsigned long)ci->last_topic_time);
			}

			/* If the channel is forbidden, add close metadata */
			if (ci->flags & CI_VERBOTEN)
			{
				fprintf(f, "MD C %s private:close:closer %s\n", ci->name, ci->forbidby);
				fprintf(f, "MD C %s private:close:reason %s\n", ci->name, ci->forbidreason);
				fprintf(f, "MD C %s private:close:timestamp %lu\n", ci->name, (unsigned long)time(NULL));
			}
		}
	}
}

void write_akills(void)
{
	int i;
	Akill *ak;

	if (!f)
		return;

	if (!akills.count)
		return;

	for (i = 0; i < akills.count; i++) {
		ak = akills.list[i];

		fprintf(f, "KL %s %s %lu %lu %s %s\n", ak->user, ak->host, (unsigned long)(ak->expires == 0 ? 0 : ak->expires - ak->seton), (unsigned long)ak->seton, ak->by, ak->reason);
		klnout++;
	}
}

int AnopeInit(int argc, char **argv)
{
	time_t ts;

	f = fopen(WHERE_TO, "w");

	if (!f)
		alog("[convert to atheme] could not open %s: %d (%s)", WHERE_TO, errno, strerror(errno));

	time(&ts);
	fprintf(f, "# Database converted at %s", ctime(&ts));
	fprintf(f, "DBV 4\n");
	write_accounts();
	write_channels();
	write_akills();
	fprintf(f, "DE %d %d %d %d\n", muout, mcout, caout, klnout);
	fprintf(f, "# End conversion.\n");

	fclose(f);

	return 0;
}

void AnopeFini(void)
{
	alog("[convert to atheme] done with converter, i take it?");
}

