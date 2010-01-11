/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains the implementation of the Atheme 0.1
 * flatfile database format, with metadata extensions.
 *
 * $Id: flatfile.c 8329 2007-05-27 13:31:59Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"backend/flatfile", true, _modinit, NULL,
	"$Id: flatfile.c 8329 2007-05-27 13:31:59Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

/* database versions */
#define DB_SHRIKE	1
#define DB_ATHEME	2

/* flatfile state */
unsigned int muout = 0, mcout = 0, caout = 0, kout = 0, xout = 0, qout = 0;

static int flatfile_db_save_myusers_cb(const char *key, void *data, void *privdata)
{
	FILE *f = (FILE *) privdata;
	myuser_t *mu = (myuser_t *) data;
	node_t *tn;

	/* MU <name> <pass> <email> <registered> <lastlogin> <failnum*> <lastfail*>
	 * <lastfailon*> <flags> <language>
	 *
	 *  * failnum, lastfail, and lastfailon are deprecated (moved to metadata)
	 */
	fprintf(f, "MU %s %s %s %ld", mu->name, mu->pass, mu->email, (long)mu->registered);
	fprintf(f, " %ld", (long)mu->lastlogin);
	fprintf(f, " 0 0 0");
	if (LIST_LENGTH(&mu->logins) > 0)
		fprintf(f, " %d", mu->flags & ~MU_NOBURSTLOGIN);
	else
		fprintf(f, " %d", mu->flags);
	fprintf(f, " %s\n", language_get_name(mu->language));

	muout++;

	LIST_FOREACH(tn, object(mu)->metadata.head)
	{
		metadata_t *md = (metadata_t *)tn->data;

		fprintf(f, "MD U %s %s %s\n", mu->name, md->name, md->value);
	}

	LIST_FOREACH(tn, mu->memos.head)
	{
		mymemo_t *mz = (mymemo_t *)tn->data;

		fprintf(f, "ME %s %s %lu %lu %s\n", mu->name, mz->sender, (unsigned long)mz->sent, (unsigned long)mz->status, mz->text);
	}

	LIST_FOREACH(tn, mu->memo_ignores.head)
	{
		fprintf(f, "MI %s %s\n", mu->name, (char *)tn->data);
	}

	LIST_FOREACH(tn, mu->access_list.head)
	{
		fprintf(f, "AC %s %s\n", mu->name, (char *)tn->data);
	}

	LIST_FOREACH(tn, mu->nicks.head)
	{
		mynick_t *mn = tn->data;

		fprintf(f, "MN %s %s %ld %ld\n", mu->name, mn->nick, (long)mn->registered, (long)mn->lastseen);
	}

	return 0; 
}

/* write atheme.db */
static void flatfile_db_save(void *arg)
{
	myuser_t *mu;
	myuser_name_t *mun;
	mychan_t *mc;
	chanacs_t *ca;
	kline_t *k;
	xline_t *x;
	qline_t *q;
	svsignore_t *svsignore;
	soper_t *soper;
	node_t *n, *tn, *tn2;
	FILE *f;
	int errno1, was_errored = 0;
	mowgli_patricia_iteration_state_t state;

	errno = 0;

	/* reset state */
	muout = 0, mcout = 0, caout = 0, kout = 0, xout = 0, qout = 0;

	/* write to a temporary file first */
	if (!(f = fopen(DATADIR "/atheme.db.new", "w")))
	{
		errno1 = errno;
		slog(LG_INFO, "db_save(): cannot create atheme.db.new: %s", strerror(errno1));
		wallops(_("\2DATABASE ERROR\2: db_save(): cannot create atheme.db.new: %s"), strerror(errno1));
		return;
	}

	/* write the database version */
	fprintf(f, "DBV 6\n");
	fprintf(f, "CF %s\n", bitmask_to_flags(ca_all, chanacs_flags));

	slog(LG_DEBUG, "db_save(): saving myusers");

	mowgli_patricia_foreach(mulist, flatfile_db_save_myusers_cb, f);

	slog(LG_DEBUG, "db_save(): saving mychans");

	MOWGLI_PATRICIA_FOREACH(mc, &state, mclist)
	{
		/* find a founder */
		mu = NULL;
		LIST_FOREACH(tn, mc->chanacs.head)
		{
			ca = (chanacs_t *)tn->data;
			if (ca->myuser != NULL && ca->level & CA_FOUNDER)
			{
				mu = ca->myuser;
				break;
			}
		}
		/* MC <name> <pass> <founder> <registered> <used> <flags>
		 * <mlock_on> <mlock_off> <mlock_limit> [mlock_key]
		 * PASS is now ignored -- replaced with a "0" to avoid having to special-case this version
		 */
		fprintf(f, "MC %s %s %s %ld %ld", mc->name, "0", mu != NULL ? mu->name : "*", (long)mc->registered, (long)mc->used);
		fprintf(f, " %d", mc->flags);
		fprintf(f, " %d", mc->mlock_on);
		fprintf(f, " %d", mc->mlock_off);
		fprintf(f, " %d", mc->mlock_limit);

		if (mc->mlock_key)
			fprintf(f, " %s\n", mc->mlock_key);
		else
			fprintf(f, "\n");

		mcout++;

		LIST_FOREACH(tn, mc->chanacs.head)
		{
			ca = (chanacs_t *)tn->data;

			fprintf(f, "CA %s %s %s %ld\n", ca->mychan->name, ca->myuser ? ca->myuser->name : ca->host,
					bitmask_to_flags(ca->level, chanacs_flags), (long)ca->tmodified);

			LIST_FOREACH(tn2, object(ca)->metadata.head)
			{
				metadata_t *md = (metadata_t *)tn2->data;

				fprintf(f, "MD A %s:%s %s %s\n", ca->mychan->name,
					(ca->myuser) ? ca->myuser->name : ca->host, md->name, md->value);
			}

			caout++;
		}

		LIST_FOREACH(tn, object(mc)->metadata.head)
		{
			metadata_t *md = (metadata_t *)tn->data;

			fprintf(f, "MD C %s %s %s\n", mc->name, md->name, md->value);
		}
	}

	/* subscriptions -- we have to walk the myuser table again
	   because otherwise we could encounter unknowns on restart. */
	slog(LG_DEBUG, "db_save(): saving subscriptions");

	MOWGLI_PATRICIA_FOREACH(mu, &state, mulist)
	{
		metadata_subscription_t *subscriptor;

		LIST_FOREACH(n, mu->subscriptions.head)
		{
			node_t *i;
			string_t *str = new_string(64);
			subscriptor = (metadata_subscription_t *) n->data;

			LIST_FOREACH(i, subscriptor->taglist.head)
			{
				str->append(str, i->data, strlen(i->data));

				if (i->next)
					str->append_char(str, ',');
			}

			fprintf(f, "SU %s %s %s\n", mu->name, subscriptor->mu->name, str->str);

			string_delete(str);
		}
	}

	/* Old names */
	MOWGLI_PATRICIA_FOREACH(mun, &state, oldnameslist)
	{
		fprintf(f, "NAM %s\n", mun->name);
		LIST_FOREACH(tn, object(mun)->metadata.head)
		{
			metadata_t *md = (metadata_t *)tn->data;

			fprintf(f, "MD N %s %s %s\n", mun->name, md->name, md->value);
		}
	}

	/* Services ignores */
	slog(LG_DEBUG, "db_save(): saving svsignores");

	LIST_FOREACH(n, svs_ignore_list.head)
	{
		svsignore = (svsignore_t *)n->data;

		/* SI <mask> <settime> <setby> <reason> */
		fprintf(f, "SI %s %ld %s %s\n", svsignore->mask, (long)svsignore->settime, svsignore->setby, svsignore->reason);
	}

	/* Services operators */
	slog(LG_DEBUG, "db_save(): saving sopers");

	LIST_FOREACH(n, soperlist.head)
	{
		soper = n->data;

		if (soper->flags & SOPER_CONF || soper->myuser == NULL)
			continue;

		/* SO <account> <operclass> <flags> [password] */
		if (soper->password != NULL)
			fprintf(f, "SO %s %s %d %s\n", soper->myuser->name, soper->classname, soper->flags, soper->password);
		else
			fprintf(f, "SO %s %s %d\n", soper->myuser->name, soper->classname, soper->flags);
	}

	slog(LG_DEBUG, "db_save(): saving klines");

	fprintf(f, "KID %lu\n", me.kline_id);

	LIST_FOREACH(n, klnlist.head)
	{
		k = (kline_t *)n->data;

		/* KL <user> <host> <duration> <settime> <setby> <reason> */
		fprintf(f, "KL %s %s %ld %ld %s %s\n", k->user, k->host, k->duration, (long)k->settime, k->setby, k->reason);

		kout++;
	}

	slog(LG_DEBUG, "db_save(): saving xlines");

	fprintf(f, "XID %lu\n", me.xline_id);

	LIST_FOREACH(n, xlnlist.head)
	{
		x = (xline_t *)n->data;

		/* XL <gecos> <duration> <settime> <setby> <reason> */
		fprintf(f, "XL %s %ld %ld %s %s\n", x->realname, x->duration, (long)x->settime, x->setby, x->reason);

		xout++;
	}

	fprintf(f, "QID %lu\n", me.qline_id);

	LIST_FOREACH(n, qlnlist.head)
	{
		q = (qline_t *)n->data;

		/* QL <mask> <duration> <settime> <setby> <reason> */
		fprintf(f, "QL %s %ld %ld %s %s\n", q->mask, q->duration, (long)q->settime, q->setby, q->reason);

		qout++;
	}

	/* DE <muout> <mcout> <caout> <kout> <xout> <qout> */
	fprintf(f, "DE %d %d %d %d %d %d\n", muout, mcout, caout, kout, xout, qout);

	was_errored = ferror(f);
	if (!was_errored)
		was_errored = fflush(f);
	/* fsync it before deleting the old */
	if (!was_errored)
		was_errored = fsync(fileno(f));
	was_errored |= fclose(f);
	if (was_errored)
	{
		errno1 = errno;
		slog(LG_INFO, "db_save(): cannot write to atheme.db.new: %s", strerror(errno1));
		wallops(_("\2DATABASE ERROR\2: db_save(): cannot write to atheme.db.new: %s"), strerror(errno1));
		return;
	}

	/* now, replace the old database with the new one, using an atomic rename */
#ifdef _WIN32
	unlink(DATADIR "/atheme.db" );
#endif
	
	if ((rename(DATADIR "/atheme.db.new", DATADIR "/atheme.db")) < 0)
	{
		errno1 = errno;
		slog(LG_INFO, "db_save(): cannot rename atheme.db.new to atheme.db: %s", strerror(errno1));
		wallops(_("\2DATABASE ERROR\2: db_save(): cannot rename atheme.db.new to atheme.db: %s"), strerror(errno1));
		return;
	}

	hook_call_db_saved();
}

/* loads atheme.db */
static void flatfile_db_load(void)
{
	myuser_t *mu, *founder = NULL;
	myuser_name_t *mun;
	mychan_t *mc;
	kline_t *k;
	xline_t *x;
	qline_t *q;
	svsignore_t *svsignore;
	unsigned int versn = 0, i;
	unsigned int linecnt = 0, muin = 0, mcin = 0, cain = 0, kin = 0, xin = 0, qin = 0;
	FILE *f;
	char *item, *s, *buf;
	size_t bufsize = BUFSIZE, n;
	int c;
	unsigned int their_ca_all = 0;

	f = fopen(DATADIR "/atheme.db", "r");
	if (f == NULL)
	{
		if (errno == ENOENT)
		{
			slog(LG_ERROR, "db_load(): %s does not exist, creating it", DATADIR "/atheme.db");
			return;
		}
		else
		{
			slog(LG_ERROR, "db_load(): can't open %s for reading: %s", DATADIR "/atheme.db", strerror(errno));
			slog(LG_ERROR, "db_load(): exiting to avoid data loss");
			exit(1);
		}
	}

	slog(LG_DEBUG, "db_load(): ----------------------- loading ------------------------");

	buf = smalloc(bufsize);

	/* start reading it, one line at a time */
	for (;;)
	{
		n = 0;
		while ((c = getc(f)) != EOF && c != '\n')
		{
			buf[n++] = c;
			if (n == bufsize)
			{
				bufsize *= 2;
				buf = srealloc(buf, bufsize);
			}
		}
		buf[n] = '\0';

		if (c == EOF && ferror(f))
		{
			slog(LG_ERROR, "db_load(): error while reading %s: %s", DATADIR "/atheme.db", strerror(errno));
			slog(LG_ERROR, "db_load(): exiting to avoid data loss");
			exit(1);
		}

		if (c == EOF && n == 0)
			break;

		linecnt++;

		/* check for unimportant lines */
		item = strtok(buf, " ");
		strip(item);

		if (item == NULL || *item == '#' || *item == '\n' || *item == '\t' || *item == ' ' || *item == '\0' || *item == '\r')
			continue;

		/* database version */
		if (!strcmp("DBV", item))
		{
			versn = atoi(strtok(NULL, " "));
			if (versn > 6)
			{
				slog(LG_INFO, "db_load(): database version is %d; i only understand 5 (Atheme 2.0, 2.1), "
					"4 (Atheme 0.2), 3 (Atheme 0.2 without CA_ACLVIEW), 2 (Atheme 0.1) or 1 (Shrike)", versn);
				exit(EXIT_FAILURE);
			}
		}
		else if (!strcmp("CF", item))
		{
			/* enabled chanacs flags */
			s = strtok(NULL, " ");
			if (s == NULL)
				slog(LG_INFO, "db_load(): missing param to CF");
			else
			{
				their_ca_all = flags_to_bitmask(s, chanacs_flags, 0);
				if (their_ca_all & ~ca_all)
				{
					slog(LG_ERROR, "db_load(): losing flags %s from file", bitmask_to_flags(their_ca_all & ~ca_all, chanacs_flags));
				}
				if (~their_ca_all & ca_all)
				{
					slog(LG_ERROR, "db_load(): making up flags %s not present in file", bitmask_to_flags(~their_ca_all & ca_all, chanacs_flags));
				}
			}
		}
		else if (!strcmp("MU", item))
		{
			/* myusers */
			char *muname, *mupass, *muemail;

			if ((s = strtok(NULL, " ")))
			{
				/* We need to know the flags before we myuser_add,
				 * so we need a few temporary places to put stuff.
				 */
				unsigned int registered, lastlogin;
				char *failnum, *lastfailaddr, *lastfailtime;
				char *flagstr, *language;

				if (myuser_find(s))
				{
					slog(LG_INFO, "db_load(): skipping duplicate account %s (line %d)", s, linecnt);
					continue;
				}

				muin++;

				muname = s;
				mupass = strtok(NULL, " ");
				muemail = strtok(NULL, " ");

				registered = atoi(strtok(NULL, " "));
				lastlogin = atoi(strtok(NULL, " "));
				failnum = strtok(NULL, " ");
				lastfailaddr = strtok(NULL, " ");
				lastfailtime = strtok(NULL, " ");
				flagstr = strtok(NULL, " ");
				language = strtok(NULL, " ");

				mu = myuser_add(muname, mupass, muemail, atol(flagstr));

				mu->registered = registered;
				mu->lastlogin = lastlogin;

				if (strcmp(failnum, "0"))
					metadata_add(mu, "private:loginfail:failnum", failnum);
				if (strcmp(lastfailaddr, "0"))
					metadata_add(mu, "private:loginfail:lastfailaddr", lastfailaddr);
				if (strcmp(lastfailtime, "0"))
					metadata_add(mu, "private:loginfail:lastfailtime", lastfailtime);

				/* Verification keys were moved to metadata,
				 * but we'll still accept them from legacy
				 * databases. Note that we only transfer over
				 * initial registration keys -- I don't think
				 * we saved mu->temp, so we can't transition
				 * e-mail change keys anyway.     --alambert
				 */
				if ((s = strtok(NULL, " ")))
				{
					strip(s);
					metadata_add(mu, "private:verify:register:key", s);
					metadata_add(mu, "private:verify:register:timestamp", "0");
				}

				/* Create entries for languages in the db
				 * even if we do not have catalogs for them.
				 */
				if (language != NULL)
					mu->language = language_add(language);
			}
		}
		else if (!strcmp("ME", item))
		{
			/* memo */
			char *sender, *text;
			time_t mtime;
			unsigned int status;
			mymemo_t *mz;

			mu = myuser_find(strtok(NULL, " "));
			sender = strtok(NULL, " ");
			mtime = atoi(strtok(NULL, " "));
			status = atoi(strtok(NULL, " "));
			text = strtok(NULL, "\n");

			if (!mu)
			{
				slog(LG_DEBUG, "db_load(): memo for unknown account");
				continue;
			}

			if (!sender || !mtime || !text)
				continue;

			mz = smalloc(sizeof(mymemo_t));

			strlcpy(mz->sender, sender, NICKLEN);
			strlcpy(mz->text, text, MEMOLEN);
			mz->sent = mtime;
			mz->status = status;

			if (!(mz->status & MEMO_READ))
				mu->memoct_new++;

			node_add(mz, node_create(), &mu->memos);
		}
		else if (!strcmp("MI", item))
		{
			/* memo ignore */
			char *user, *target, *strbuf;

			user = strtok(NULL, " ");
			target = strtok(NULL, "\n");

			mu = myuser_find(user);
			
			if (!mu)
			{
				slog(LG_DEBUG, "db_load(): invalid ignore (MI %s %s)", user,target);
				continue;
			}
			
			strbuf = sstrdup(target);
			
			node_add(strbuf, node_create(), &mu->memo_ignores);
		}
		else if (!strcmp("AC", item))
		{
			/* myuser access list */
			char *user, *mask;

			user = strtok(NULL, " ");
			mask = strtok(NULL, "\n");

			mu = myuser_find(user);

			if (mu == NULL)
			{
				slog(LG_DEBUG, "db_load(): invalid access entry<%s> for unknown user<%s>", mask, user);
				continue;
			}

			myuser_access_add(mu, mask);
		}
		else if (!strcmp("MN", item))
		{
			/* registered nick */
			char *user, *nick, *treg, *tseen;
			mynick_t *mn;

			user = strtok(NULL, " ");
			nick = strtok(NULL, " ");
			treg = strtok(NULL, " ");
			tseen = strtok(NULL, " ");

			mu = myuser_find(user);
			if (mu == NULL || nick == NULL || tseen == NULL)
			{
				slog(LG_DEBUG, "db_load(): invalid registered nick<%s> for user<%s>", nick, user);
				continue;
			}

			if (mynick_find(nick))
			{
				slog(LG_INFO, "db_load(): skipping duplicate nick %s (account %s) (line %d)", nick, user, linecnt);
				continue;
			}

			mn = mynick_add(mu, nick);
			mn->registered = atoi(treg);
			mn->lastseen = atoi(tseen);
		}
		else if (!strcmp("SU", item))
		{
			/* subscriptions */
			char *user, *sub_user, *tags, *tag;
			myuser_t *subscriptor;
			metadata_subscription_t *md;

			user = strtok(NULL, " ");
			sub_user = strtok(NULL, " ");
			tags = strtok(NULL, "\n");
			if (!user || !sub_user || !tags)
			{
				slog(LG_INFO, "db_load(): invalid subscription (line %d)", linecnt);
				continue;
			}

			strip(tags);

			mu = myuser_find(user);
			subscriptor = myuser_find(sub_user);
			if (!mu || !subscriptor)
			{
				slog(LG_INFO, "db_load(): invalid subscription <%s,%s> (line %d)", user, sub_user, linecnt);
				continue;
			}

			md = smalloc(sizeof(metadata_subscription_t));
			md->mu = subscriptor;

			tag = strtok(tags, ",");
			do
			{
				node_add(sstrdup(tag), node_create(), &md->taglist);
			} while ((tag = strtok(NULL, ",")) != NULL);

			node_add(md, node_create(), &mu->subscriptions);
		}
		else if (!strcmp("NAM", item))
		{
			/* formerly registered name (created by a marked account being dropped) */
			char *user;

			user = strtok(NULL, " \n");
			if (!user)
			{
				slog(LG_INFO, "db_load(): invalid old name (line %d)", linecnt);
				continue;
			}
			myuser_name_add(user);
		}
		else if (!strcmp("SO", item))
		{
			/* services oper */
			char *user, *class, *flagstr, *password;

			user = strtok(NULL, " ");
			class = strtok(NULL, " ");
			flagstr = strtok(NULL, " \n");
			password = strtok(NULL, "\n");

			mu = myuser_find(user);

			if (!mu || !class || !flagstr)
			{
				slog(LG_DEBUG, "db_load(): invalid services oper (SO %s %s %s)", user, class, flagstr);
				continue;
			}
			soper_add(mu->name, class, atoi(flagstr) & ~SOPER_CONF, password);
		}
		else if (!strcmp("MC", item))
		{
			/* mychans */
			char *mcname;

			if ((s = strtok(NULL, " ")))
			{
				if (mychan_find(s))
				{
					slog(LG_INFO, "db_load(): skipping duplicate channel %s (line %d)", s, linecnt);
					continue;
				}

				mcin++;

				mcname = s;
				/* unused (old password) */
				(void)strtok(NULL, " ");

				mc = mychan_add(mcname);

				founder = myuser_find(strtok(NULL, " "));

				mc->registered = atoi(strtok(NULL, " "));
				mc->used = atoi(strtok(NULL, " "));
				mc->flags = atoi(strtok(NULL, " "));

				mc->mlock_on = atoi(strtok(NULL, " "));
				mc->mlock_off = atoi(strtok(NULL, " "));
				mc->mlock_limit = atoi(strtok(NULL, " "));

				if ((s = strtok(NULL, " ")))
				{
					strip(s);
					if (*s != '\0' && *s != ':' && !strchr(s, ','))
						mc->mlock_key = sstrdup(s);
				}

				if (versn < 5 && config_options.join_chans)
					mc->flags |= MC_GUARD;
			}
		}
		else if (!strcmp("MD", item))
		{
			/* Metadata entry */
			char *type = strtok(NULL, " ");
			char *name = strtok(NULL, " ");
			char *property = strtok(NULL, " ");
			char *value = strtok(NULL, "");

			if (!type || !name || !property || !value)
				continue;

			strip(value);

			if (type[0] == 'U')
			{
				mu = myuser_find(name);
				if (mu != NULL)
					metadata_add(mu, property, value);
			}
			else if (type[0] == 'C')
			{
				mc = mychan_find(name);
				if (mc != NULL)
					metadata_add(mc, property, value);
			}
			else if (type[0] == 'A')
			{
				chanacs_t *ca;
				char *mask;

				mask = strrchr(name, ':');
				if (mask != NULL)
				{
					*mask++ = '\0';
					ca = chanacs_find_by_mask(mychan_find(name), mask, CA_NONE);
					if (ca != NULL)
						metadata_add(ca, property, value);
				}
			}
			else if (type[0] == 'N')
			{
				mun = myuser_name_find(name);
				if (mun != NULL)
					metadata_add(mun, property, value);
			}
			else
				slog(LG_DEBUG, "db_load(): unknown metadata type %s", type);
		}
		else if (!strcmp("UR", item))
		{
			/* Channel URLs (obsolete) */
			char *chan, *url;

			chan = strtok(NULL, " ");
			url = strtok(NULL, " ");

			strip(url);

			if (chan && url)
			{
				mc = mychan_find(chan);

				if (mc)
					metadata_add(mc, "url", url);
			}
		}
		else if (!strcmp("EM", item))
		{
			/* Channel entry messages (obsolete) */
			char *chan, *message;

			chan = strtok(NULL, " ");
			message = strtok(NULL, "");

			strip(message);

			if (chan && message)
			{
				mc = mychan_find(chan);

				if (mc)
					metadata_add(mc, "private:entrymsg", message);
			}
		}
		else if (!strcmp("CA", item))
		{
			/* chanacs */
			chanacs_t *ca;
			char *cachan, *causer;

			cachan = strtok(NULL, " ");
			causer = strtok(NULL, " ");

			if (cachan && causer)
			{
				mc = mychan_find(cachan);
				mu = myuser_find(causer);

				if (mc == NULL || (mu == NULL && !validhostmask(causer)))
				{
					slog(LG_ERROR, "db_load(): invalid chanacs (line %d)", linecnt);
					continue;
				}

				cain++;

				if (versn >= DB_ATHEME)
				{
					unsigned int fl = flags_to_bitmask(strtok(NULL, " "), chanacs_flags, 0x0);
					const char *tsstr;
					time_t ts = 0;

					/* Compatibility with oldworld Atheme db's. --nenolod */
					/* arbitrary cutoff to avoid touching newer +voOt entries -- jilles */
					if (fl == OLD_CA_AOP && versn < 4)
						fl = CA_AOP_DEF;

					/* 
					 * If the database revision is version 6 or newer, CA entries are
					 * timestamped... otherwise we use 0 as the last modified TS
					 *    --nenolod/jilles
					 */
					tsstr = strtok(NULL, " ");
					if (tsstr != NULL)
						ts = atoi(tsstr);

					/* previous to CA_ACLVIEW, everyone could view
					 * access lists. If they aren't AKICKed, upgrade
					 * them. This keeps us from breaking old XOPs.
					 */
					if (versn < 4)
						if (!(fl & CA_AKICK))
							fl |= CA_ACLVIEW;

					if (their_ca_all == 0)
					{
						their_ca_all = CA_VOICE | CA_AUTOVOICE | CA_OP | CA_AUTOOP | CA_TOPIC | CA_SET | CA_REMOVE | CA_INVITE | CA_RECOVER | CA_FLAGS | CA_HALFOP | CA_AUTOHALFOP | CA_ACLVIEW | CA_AKICK;
						slog(LG_INFO, "db_load(): old database, making up flags %s", bitmask_to_flags(~their_ca_all & ca_all, chanacs_flags));
					}

					/* Grant +h if they have +o,
					 * the file does not have +h enabled
					 * and we currently have +h enabled.
					 * This preserves AOP, SOP and +*.
					 */
					if (fl & CA_OP && !(their_ca_all & CA_HALFOP) && ca_all & CA_HALFOP)
						fl |= CA_HALFOP;

					/* Set new-style founder flag */
					if (founder != NULL && mu == founder && !(their_ca_all & CA_FOUNDER))
						fl |= CA_FOUNDER;

					/* Set new-style +q and +a flags */
					if (fl & CA_SET && fl & CA_OP && !(their_ca_all & CA_USEPROTECT) && ca_all & CA_USEPROTECT)
						fl |= CA_USEPROTECT;
					if (fl & CA_FOUNDER && !(their_ca_all & CA_USEOWNER) && ca_all & CA_USEOWNER)
						fl |= CA_USEOWNER;

					if ((!mu) && (validhostmask(causer)))
						ca = chanacs_add_host(mc, causer, fl, ts);
					else
						ca = chanacs_add(mc, mu, fl, ts);
				}
				else if (versn == DB_SHRIKE)	/* DB_SHRIKE */
				{
					unsigned int fl = atol(strtok(NULL, " "));
					unsigned int fl2 = 0x0;
					time_t ts = CURRTIME;

					switch (fl)
					{
					  case SHRIKE_CA_VOP:
						  fl2 = chansvs.ca_vop;
						  break;
					  case SHRIKE_CA_AOP:
						  fl2 = chansvs.ca_aop;
						  break;
					  case SHRIKE_CA_SOP:
						  fl2 = chansvs.ca_sop;
						  break;
					  case SHRIKE_CA_SUCCESSOR:
						  fl2 = CA_SUCCESSOR_0;
						  break;
					  case SHRIKE_CA_FOUNDER:
						  fl2 = CA_FOUNDER_0;
						  break;
					}

					if ((!mu) && (validhostmask(causer)))
						ca = chanacs_add_host(mc, causer, fl2, ts);
					else
						ca = chanacs_add(mc, mu, fl2, ts);
				}
			}
		}
		else if (!strcmp("SI", item))
		{
				/* Services ignores */
			char *mask, *setby, *reason, *tmp;
			time_t settime;

			mask = strtok(NULL, " ");
                        tmp = strtok(NULL, " ");
			settime = atol(tmp);
			setby = strtok(NULL, " ");
			reason = strtok(NULL, "");

			strip(reason);

			svsignore = svsignore_add(mask, reason);
			svsignore->settime = settime;
			svsignore->setby = sstrdup(setby);

		}
		else if (!strcmp("KID", item))
		{
			/* unique kline id */
			char *id = strtok(NULL, " ");
			me.kline_id = atol(id);
		}
		else if (!strcmp("KL", item))
		{
			/* klines */
			char *user, *host, *reason, *setby, *tmp;
			time_t settime;
			long duration;

			user = strtok(NULL, " ");
			host = strtok(NULL, " ");
			tmp = strtok(NULL, " ");
			duration = atol(tmp);
			tmp = strtok(NULL, " ");
			settime = atol(tmp);
			setby = strtok(NULL, " ");
			reason = strtok(NULL, "");

			strip(reason);

			k = kline_add(user, host, reason, duration, setby);
			k->settime = settime;
			/* XXX this is not nice, oh well -- jilles */
			k->expires = k->settime + k->duration;

			kin++;
		}
		else if (!strcmp("XID", item))
		{
			/* unique xline id */
			char *id = strtok(NULL, " ");
			me.xline_id = atol(id);
		}
		else if (!strcmp("XL", item))
		{
			char *realname, *reason, *setby, *tmp;
			time_t settime;
			long duration;

			realname = strtok(NULL, " ");
			tmp = strtok(NULL, " ");
			duration = atol(tmp);
			tmp = strtok(NULL, " ");
			settime = atol(tmp);
			setby = strtok(NULL, " ");
			reason = strtok(NULL, "");

			strip(reason);

			x = xline_add(realname, reason, duration, setby);
			x->settime = settime;

			/* XXX this is not nice, oh well -- jilles */
			x->expires = x->settime + x->duration;

			xin++;
		}
		else if (!strcmp("QID", item))
		{
			/* unique qline id */
			char *id = strtok(NULL, " ");
			me.qline_id = atol(id);
		}
		else if (!strcmp("QL", item))
		{
			char *mask, *reason, *setby, *tmp;
			time_t settime;
			long duration;

			mask = strtok(NULL, " ");
			tmp = strtok(NULL, " ");
			duration = atol(tmp);
			tmp = strtok(NULL, " ");
			settime = atol(tmp);
			setby = strtok(NULL, " ");
			reason = strtok(NULL, "");

			strip(reason);

			q = qline_add(mask, reason, duration, setby);
			q->settime = settime;

			/* XXX this is not nice, oh well -- jilles */
			q->expires = q->settime + q->duration;

			qin++;
		}
		else if (!strcmp("DE", item))
		{
			/* end */
			i = atoi(strtok(NULL, " "));
			if (i != muin)
				slog(LG_ERROR, "db_load(): got %d myusers; expected %d", muin, i);

			i = atoi(strtok(NULL, " "));
			if (i != mcin)
				slog(LG_ERROR, "db_load(): got %d mychans; expected %d", mcin, i);

			i = atoi(strtok(NULL, " "));
			if (i != cain)
				slog(LG_ERROR, "db_load(): got %d chanacs; expected %d", cain, i);

			if ((s = strtok(NULL, " ")))
				if ((i = atoi(s)) != kin)
					slog(LG_ERROR, "db_load(): got %d klines; expected %d", kin, i);

			if ((s = strtok(NULL, " ")))
				if ((i = atoi(s)) != xin)
					slog(LG_ERROR, "db_load(): got %d xlines; expected %d", xin, i);

			if ((s = strtok(NULL, " ")))
				if ((i = atoi(s)) != qin)
					slog(LG_ERROR, "db_load(): got %d qlines; expected %d", qin, i);
		}
	}

	fclose(f);

	free(buf);

	slog(LG_DEBUG, "db_load(): ------------------------- done -------------------------");
}

void _modinit(module_t *m)
{
	m->mflags = MODTYPE_CORE;

	db_load = &flatfile_db_load;
	db_save = &flatfile_db_save;

	backend_loaded = true;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
