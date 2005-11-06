/*
 * Copyright (c) 2005 William Pitcock, et al.
 * The rights to this code are as documented in doc/LICENSE.
 * 
 * IRCServices datafile manipulation routines.
 * 
 * $Id: dbtool.c 3607 2005-11-06 23:57:17Z jilles $
 */

#include "atheme.h"

#ifdef _WIN32
#define SIGUSR1 0
#endif

/* these two macros stolen from anope */
#define READ_BUFFER(buf,f)      { READ_DB((f),(buf),sizeof(buf)); }
#define READ_DB(f,buf,len)      (fread((buf),1,(len),(f)))

/* this is my own */
#define DEBUG(x) if (verbose_r) { x }

boolean_t verbose_r = FALSE;

static void *
my_scalloc(size_t elsize, size_t els)
{
	void *buf = calloc(elsize, els);

	if (!buf)
		raise(SIGUSR1);
	return buf;
}

struct anope_nickentry {
	char     *nick;
	char     *pass;
	char     *email;
	char     *greet; 
	uint32_t  icq;
	char     *url;
	uint32_t  flags;
	uint16_t  locale;
	uint16_t  hostmasks;
	char    **access;
	uint16_t  memocount;
	uint16_t  memomax;
	uint16_t  registrations;
	uint16_t  reg_max;
	uint16_t  unknown;
	uint32_t  unknown2;
	uint16_t  unknown3;
	char     *unknown4;
};

struct anope_chanaccess {
	uint16_t in_use;
	uint16_t level;
	char	*nick;
	uint32_t last_seen;
};

struct anope_chanentry {
	char	 name[64];
	char	*founder;
	char	*successor;
	char	 founderpass[32];
	char	*description;
	char	*url;
	char	*email;
	uint32_t registered;
	uint32_t lastuse;
	char	*topic;
	char	 topic_setter[32];
	uint32_t topic_time;
	uint32_t flags;
	char	*forbid_by;
	char	*forbid_reason;
	uint16_t bantype;
	uint16_t levelcount;
	uint16_t accesscount;
	struct anope_chanaccess *access;
	uint16_t akickcount;
	char   **akicks;
	uint32_t mlock_on;
	uint32_t mlock_off;
	uint32_t mlock_limit;
	char	*mlock_key;
	char	*mlock_flood;
	char	*mlock_forward;
	uint16_t memocount;
	uint16_t memomax;
	char	*entrymsg;

	/* In the file, botserv options follow. We don't have that, so whatever. */
};

/*
 * Anope levels to Atheme flagset conversion.
 */
static char *cflags[10] = {
	"+",
	"+V",
	"+vV",
	"+voO",
	"+voOt",
	"+voOti",
	"+voOtsi",
	"+voOtsir",
	"+voOtsirR",
	"+voOtsirRf"
};

static int 
get_file_version(FILE * f)
{
	int fversion =
		fgetc(f) << 24 | fgetc(f) << 16 | fgetc(f) << 8 | fgetc(f);

	if (ferror(f)) {
		return 0;
	} else if (feof(f)) {
		return 0;
	} else if (fversion < 1) {
		return 0;
	}
	return fversion;
}

static int 
read_int16(uint16_t * ret, FILE * f)
{
	int c1, c2;

	c1 = fgetc(f);
	c2 = fgetc(f);
	if (c1 == EOF || c2 == EOF)
		return -1;
	*ret = c1 << 8 | c2;
	return 0;
}

static int 
read_int32(uint32_t * ret, FILE * f)
{
	int c1, c2, c3, c4;

	c1 = fgetc(f);
	c2 = fgetc(f);
	c3 = fgetc(f);
	c4 = fgetc(f);
	if (c1 == EOF || c2 == EOF || c3 == EOF || c4 == EOF)
		return -1;
	*ret = c1 << 24 | c2 << 16 | c3 << 8 | c4;
	return 0;
}

static int 
read_string(char **ret, FILE * f)
{
	char     *s;
	uint16_t  len;

	if (read_int16(&len, f) < 0)
		return -1;
	if (len == 0) {
		*ret = NULL;
		return 0;
	}
	s = my_scalloc(len, 1);
	if (len != fread(s, 1, len, f)) {
		free(s);
		return -1;
	}
	*ret = s;
	return 0;
}

static int
parse_nickserv_db(char *filepath)
{
	FILE *f = fopen(filepath, "r");
	struct anope_nickentry *nc;
	uint32_t i, c, j, x, ver = 0;
	uint16_t z;
	char *junk;
	char junkbuf[512];
	nc = my_scalloc(1, sizeof(struct anope_nickentry));
	ver = get_file_version(f);

	if (ver < 13)
	{
		printf("### Error: Please run your database through an updated anope.");
		return -1;
	}

	if (!f)
	{
		printf("### Error: Cannot open %s (%d = %s)\n",
				filepath, errno, strerror(errno));
		return -1;
	}

	if (verbose_r)
	{
		printf("# Parsing NickServ datafile: %s\n", filepath);
		printf("# nick.db database version: %d\n", ver);
	}

	for (i = 0; i < 1024; i++)
	{
		while ((c = fgetc(f)) == 1)
		{
			memset(nc, 0, sizeof(nc));

			/* Lets do it! */
			read_string(&nc->nick, f);
			read_string(&nc->pass, f);
			read_string(&nc->email, f);
			read_string(&nc->greet, f);
			read_int32(&nc->icq, f);
			read_string(&nc->url, f);
			read_int32(&nc->flags, f);
			read_int16(&nc->locale, f);

			read_int16(&nc->hostmasks, f);
			if (nc->hostmasks)
			{
				char **access_p;
				access_p = my_scalloc(sizeof(char *) * nc->hostmasks, 1);
				nc->access = access_p;
				read_string(access_p, f);
			}

			read_int16(&nc->memocount, f);
			read_int16(&nc->memomax, f);

			if (nc->memocount)
			{
				for (j = 0; j < nc->memocount; j++)
				{
					read_int32(&x, f);
					read_int16(&z, f);
					read_int32(&x, f);
					READ_BUFFER(&junkbuf, f);
					read_string(&junk, f);
				}
			}

			/* Sanity checking. */
			if (!nc->email)
				nc->email = strdup("unknown@unknown.com");

			printf("MU %s %s %s %ld 0 0 0 0 0\n", nc->nick, nc->pass, nc->email, time(NULL));
		}
	}
	fclose(f);
	return 0;
}

static int
parse_chanserv_db(char *filepath)
{
	FILE *f = fopen(filepath, "r");
	struct anope_chanentry *ci;
	uint32_t i, c, j, x, ver = 0;
	uint16_t n_ttb;
	uint16_t z;
	char *junk;
	char junkbuf[512];
	ci = my_scalloc(sizeof(struct anope_chanentry), 1);
	ver = get_file_version(f);

	if (ver < 16)
	{
		printf("### Error: Please run your database through an updated anope.");
		return -1;
	}

	if (!f)
	{
		printf("### Error: Cannot open %s (%d = %s)\n",
				filepath, errno, strerror(errno));
		return -1;
	}

	if (verbose_r)
	{
		printf("# Parsing ChanServ datafile: %s\n", filepath);
		printf("# chan.db database version: %d\n", ver);
	}

	for (i = 0; i < 256; i++)
	{
		while ((c = fgetc(f)) == 1)
		{
			memset(ci, 0, sizeof(ci));

			READ_BUFFER(ci->name, f);
			read_string(&ci->founder, f);
			read_string(&ci->successor, f);
			READ_BUFFER(ci->founderpass, f);
			read_string(&ci->description, f);
			read_string(&ci->url, f);
			read_string(&ci->email, f);
			read_int32(&ci->registered, f);
			read_int32(&ci->lastuse, f);
			read_string(&ci->topic, f);
			READ_BUFFER(ci->topic_setter, f);
			read_int32(&ci->topic_time, f);
			read_int32(&ci->flags, f);
			read_string(&ci->forbid_by, f);
			read_string(&ci->forbid_reason, f);
			read_int16(&ci->bantype, f);
			read_int16(&ci->levelcount, f);

			if (ci->levelcount)
			{
				for (j = 0; j < ci->levelcount; j++)
					read_int16(&z, f);
			}

			read_int16(&ci->accesscount, f);
			if (ci->accesscount)
			{
				ci->access = my_scalloc(sizeof(struct anope_chanaccess), ci->accesscount);
				for (j = 0; j < ci->accesscount; j++)
				{
					read_int16(&ci->access[j].in_use, f);
					if (&ci->access[j].in_use)
					{
						read_int16(&ci->access[j].level, f);
						read_string(&ci->access[j].nick, f);
						read_int32(&ci->access[j].last_seen, f);
					}
				}
			}

			read_int16(&ci->akickcount, f);
			if (ci->akickcount)
			{
				ci->akicks = my_scalloc(sizeof(char *) * ci->akickcount, 1);
				for (j = 0; j < ci->akickcount; j++)
				{
					read_int16(&z, f);
					read_string(&ci->akicks[j], f);
					read_string(&junk, f);
					read_string(&junk, f);
					read_int32(&x, f);
				}
			}

			read_int32(&ci->mlock_on, f);
			read_int32(&ci->mlock_off, f);
			read_int32(&ci->mlock_limit, f);
			read_string(&ci->mlock_key, f);
			read_string(&ci->mlock_flood, f);
			read_string(&ci->mlock_forward, f);

			read_int16(&ci->memocount, f);
			read_int16(&ci->memomax, f);

			if (ci->memocount)
			{
				for (j = 0; j < ci->memocount; j++)
				{
					read_int32(&x, f);
					read_int16(&z, f);
					read_int32(&x, f);
					READ_BUFFER(junkbuf, f);
					read_string(&junk, f);
				}
			}

			read_string(&ci->entrymsg, f);

			/* Botserv stuff. Just grok through it. */
			read_string(&junk, f);
			read_int32(&x, f);
			read_int16(&n_ttb, f);

			if (n_ttb)
			{
				for (j = 0; j < n_ttb; j++)
					read_int16(&z, f);
			}

			read_int16(&z, f);
			read_int16(&z, f);
			read_int16(&z, f);
			read_int16(&z, f);
			read_int16(&z, f);

			read_int16(&n_ttb, f);

			if (n_ttb)
			{
				for (j = 0; j < n_ttb; j++)
				{
					read_int16(&z, f);
					read_string(&junk, f);
					read_int16(&z, f);
				}
			}

			printf("MC %s %s %s %d %d 0 0 0 %d%s%s\n",
				ci->name, ci->founderpass, ci->founder, ci->registered,
				ci->lastuse, ci->mlock_limit, ci->mlock_key ? " " : "", 
				ci->mlock_key ? ci->mlock_key : "");

			if (ci->url)
				printf("UR %s %s\n", ci->name, ci->url);

			if (ci->entrymsg)
				printf("EM %s %s\n", ci->name, ci->entrymsg);

			if (ci->accesscount)
			{
				for (j = 0; j < ci->accesscount; j++)
				{
					printf("CA %s %s %s\n", ci->name, ci->access[j].nick, cflags[(ci->access[j].level - 1)]);
				}
			}

			if (ci->akickcount)
			{
				for (j = 0; j < ci->akickcount; j++)
					printf("CA %s %s +b\n", ci->name, ci->akicks[j]);
			}
		}
	}

	return 0;
}	

int
main(int argc, char *argv[])
{
	char pbuf[512];

	if (!argv[1])
	{
		printf("usage: %s <path to anope dir> [-verbose] > atheme.db\n", argv[0]);
		return -1;
	}

	if (argv[2] && !strcasecmp(argv[2], "-verbose"))
		verbose_r = TRUE;

	snprintf(pbuf, 512, "%s/nick.db", argv[1]);
	parse_nickserv_db(pbuf);
	snprintf(pbuf, 512, "%s/chan.db", argv[1]);
	parse_chanserv_db(pbuf);

	return 0;
}


