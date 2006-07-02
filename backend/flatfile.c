/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains the implementation of the Atheme 0.1
 * flatfile database format, with metadata extensions.
 *
 * $Id: flatfile.c 5634 2006-07-02 00:05:59Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"backend/flatfile", TRUE, _modinit, NULL,
	"$Id: flatfile.c 5634 2006-07-02 00:05:59Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

/* write atheme.db */
static void flatfile_db_save(void *arg)
{
	myuser_t *mu;
	mychan_t *mc;
	chanacs_t *ca;
	kline_t *k;
	svsignore_t *svsignore;
	node_t *n, *tn, *tn2;
	FILE *f;
	uint32_t i, muout = 0, mcout = 0, caout = 0, kout = 0;
	int errno1, was_errored = 0;

	errno = 0;

	/* write to a temporary file first */
	if (!(f = fopen("etc/atheme.db.new", "w")))
	{
		errno1 = errno;
		slog(LG_ERROR, "db_save(): cannot create atheme.db.new: %s", strerror(errno1));
		wallops("\2DATABASE ERROR\2: db_save(): cannot create atheme.db.new: %s", strerror(errno1));
		return;
	}

	/* write the database version */
	fprintf(f, "DBV 4\n");

	slog(LG_DEBUG, "db_save(): saving myusers");

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mulist[i].head)
		{
			mu = (myuser_t *)n->data;

			/* MU <name> <pass> <email> <registered> [lastlogin] [failnum*] [lastfail*]
			 * [lastfailon*] [flags]
			 *
			 *  * failnum, lastfail, and lastfailon are deprecated (moved to metadata)
			 */
			fprintf(f, "MU %s %s %s %ld", mu->name, mu->pass, mu->email, (long)mu->registered);

			if (mu->lastlogin)
				fprintf(f, " %ld", (long)mu->lastlogin);
			else
				fprintf(f, " 0");

			fprintf(f, " 0 0 0");

			if (mu->flags)
				fprintf(f, " %d\n", mu->flags);
			else
				fprintf(f, " 0\n");

			muout++;

			LIST_FOREACH(tn, mu->metadata.head)
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
		}
	}

	slog(LG_DEBUG, "db_save(): saving mychans");

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mclist[i].head)
		{
			mc = (mychan_t *)n->data;

			/* MC <name> <pass> <founder> <registered> [used] [flags]
			 * [mlock_on] [mlock_off] [mlock_limit] [mlock_key]
			 * PASS is now ignored -- replaced with a "0" to avoid having to special-case this version
			 */
			fprintf(f, "MC %s %s %s %ld %ld", mc->name, "0", mc->founder->name, (long)mc->registered, (long)mc->used);

			if (mc->flags)
				fprintf(f, " %d", mc->flags);
			else
				fprintf(f, " 0");

			if (mc->mlock_on)
				fprintf(f, " %d", mc->mlock_on);
			else
				fprintf(f, " 0");

			if (mc->mlock_off)
				fprintf(f, " %d", mc->mlock_off);
			else
				fprintf(f, " 0");

			if (mc->mlock_limit)
				fprintf(f, " %d", mc->mlock_limit);
			else
				fprintf(f, " 0");

			if (mc->mlock_key)
				fprintf(f, " %s\n", mc->mlock_key);
			else
				fprintf(f, "\n");

			mcout++;

			LIST_FOREACH(tn, mc->chanacs.head)
			{
				ca = (chanacs_t *)tn->data;

				fprintf(f, "CA %s %s %s\n", ca->mychan->name, ca->myuser ? ca->myuser->name : ca->host,
						bitmask_to_flags(ca->level, chanacs_flags));

				LIST_FOREACH(tn2, ca->metadata.head)
				{
					metadata_t *md = (metadata_t *)tn2->data;

					fprintf(f, "MD A %s:%s %s %s\n", ca->mychan->name,
						(ca->host) ? ca->host : ca->myuser->name, md->name, md->value);
				}

				caout++;
			}

			LIST_FOREACH(tn, mc->metadata.head)
			{
				metadata_t *md = (metadata_t *)tn->data;

				fprintf(f, "MD C %s %s %s\n", mc->name, md->name, md->value);
			}
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


	slog(LG_DEBUG, "db_save(): saving klines");

	LIST_FOREACH(n, klnlist.head)
	{
		k = (kline_t *)n->data;

		/* KL <user> <host> <duration> <settime> <setby> <reason> */
		fprintf(f, "KL %s %s %ld %ld %s %s\n", k->user, k->host, k->duration, (long)k->settime, k->setby, k->reason);

		kout++;
	}

	/* DE <muout> <mcout> <caout> <kout> */
	fprintf(f, "DE %d %d %d %d\n", muout, mcout, caout, kout);

	was_errored = ferror(f);
	was_errored |= fclose(f);
	if (was_errored)
	{
		errno1 = errno;
		slog(LG_ERROR, "db_save(): cannot write to atheme.db.new: %s", strerror(errno1));
		wallops("\2DATABASE ERROR\2: db_save(): cannot write to atheme.db.new: %s", strerror(errno1));
		return;
	}
	/* now, replace the old database with the new one, using an atomic rename */
	
#ifdef _WIN32
	unlink( "etc/atheme.db" );
#endif
	
	if ((rename("etc/atheme.db.new", "etc/atheme.db")) < 0)
	{
		errno1 = errno;
		slog(LG_ERROR, "db_save(): cannot rename atheme.db.new to atheme.db: %s", strerror(errno1));
		wallops("\2DATABASE ERROR\2: db_save(): cannot rename atheme.db.new to atheme.db: %s", strerror(errno1));
		return;
	}
}

/* loads atheme.db */
static void flatfile_db_load(void)
{
	myuser_t *mu;
	mychan_t *mc;
	kline_t *k;
	svsignore_t *svsignore;
	uint32_t i = 0, linecnt = 0, muin = 0, mcin = 0, cain = 0, kin = 0;
	FILE *f;
	char *item, *s, dBuf[BUFSIZE];

	f = fopen("etc/atheme.db", "r");
	if (f == NULL)
	{
		slog(LG_ERROR, "db_load(): can't open atheme.db for reading: %s", strerror(errno));
		return;
	}

	slog(LG_DEBUG, "db_load(): ----------------------- loading ------------------------");

	/* start reading it, one line at a time */
	while (fgets(dBuf, BUFSIZE, f))
	{
		linecnt++;

		/* check for unimportant lines */
		item = strtok(dBuf, " ");
		strip(item);

		if (*item == '#' || *item == '\n' || *item == '\t' || *item == ' ' || *item == '\0' || *item == '\r' || !*item)
			continue;

		/* database version */
		if (!strcmp("DBV", item))
		{
			i = atoi(strtok(NULL, " "));
			if (i > 4)
			{
				slog(LG_INFO, "db_load(): database version is %d; i only understand 4 (Atheme 0.2), 3 (Atheme 0.2 without CA_ACLVIEW), 2 (Atheme 0.1) or 1 (Shrike)", i);
				exit(EXIT_FAILURE);
			}
		}

		/* myusers */
		if (!strcmp("MU", item))
		{
			char *muname, *mupass, *muemail;

			if ((s = strtok(NULL, " ")))
			{
				/* We need to know the flags before we myuser_add,
				 * so we need a few temporary places to put stuff.
				 */
				uint32_t registered, lastlogin;
				char *failnum, *lastfailaddr, *lastfailtime;

				if ((mu = myuser_find(s)))
					continue;

				muin++;

				muname = s;
				mupass = strtok(NULL, " ");
				muemail = strtok(NULL, " ");

				registered = atoi(strtok(NULL, " "));
				lastlogin = atoi(strtok(NULL, " "));
				failnum = strtok(NULL, " ");
				lastfailaddr = strtok(NULL, " ");
				lastfailtime = strtok(NULL, " ");

				mu = myuser_add(muname, mupass, muemail, atol(strtok(NULL, " ")));

				mu->registered = registered;
				mu->lastlogin = lastlogin;

				if (strcmp(failnum, "0"))
					metadata_add(mu, METADATA_USER, "private:loginfail:failnum", failnum);
				if (strcmp(lastfailaddr, "0"))
					metadata_add(mu, METADATA_USER, "private:loginfail:lastfailaddr", lastfailaddr);
				if (strcmp(lastfailtime, "0"))
					metadata_add(mu, METADATA_USER, "private:loginfail:lastfailtime", lastfailtime);

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
					metadata_add(mu, METADATA_USER, "private:verify:register:key", s);
					metadata_add(mu, METADATA_USER, "private:verify:register:timestamp", "0");
				}
			}
		}

		if (!strcmp("ME", item))
		{
			char *sender, *text;
			time_t mtime;
			uint32_t status;
			mymemo_t *mz;

			mu = myuser_find(strtok(NULL, " "));
			sender = strtok(NULL, " ");
			mtime = atoi(strtok(NULL, " "));
			status = atoi(strtok(NULL, " "));
			text = strtok(NULL, "\n");

			if (!mu)
			{
				slog(LG_DEBUG, "db_load(): WTF -- memo for unknown account");
				continue;
			}

			if (!sender || !mtime || !text)
				continue;

			mz = smalloc(sizeof(mymemo_t));

			strlcpy(mz->sender, sender, NICKLEN);
			strlcpy(mz->text, text, MEMOLEN);
			mz->sent = mtime;
			mz->status = status;

			if (mz->status == MEMO_NEW)
				mu->memoct_new++;

			node_add(mz, node_create(), &mu->memos);
		}
		
		if (!strcmp("MI", item))
		{
			char *user, *target, *strbuf;
			myuser_t *tmu;

			user = strtok(NULL, " ");
			target = strtok(NULL, "\n");

			mu = myuser_find(user);
			tmu = myuser_find(target);
			
			if (!mu)
			{
				slog(LG_DEBUG, "db_load(): invalid ignore (MI %s %s)",user,target);
				continue;
			}
			
			strbuf = smalloc(sizeof(char[NICKLEN]));
			strlcpy(strbuf,target,NICKLEN-1);
			
			node_add(strbuf, node_create(), &mu->memo_ignores);
		}

		/* mychans */
		if (!strcmp("MC", item))
		{
			char *mcname, *mcpass;

			if ((s = strtok(NULL, " ")))
			{
				if ((mc = mychan_find(s)))
					continue;

				mcin++;

				mcname = s;
				/* unused */
				mcpass = strtok(NULL, " ");

				mc = mychan_add(mcname);

				mc->founder = myuser_find(strtok(NULL, " "));

				if (!mc->founder)
				{
					slog(LG_DEBUG, "db_load(): channel %s has no founder, dropping.", mc->name);
					mychan_delete(mc->name);
					continue;
				}

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
			}
		}

		/* Metadata entry */
		if (!strcmp("MD", item))
		{
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
					metadata_add(mu, METADATA_USER, property, value);
			}
			else if (type[0] == 'C')
			{
				mc = mychan_find(name);
				if (mc != NULL)
					metadata_add(mc, METADATA_CHANNEL, property, value);
			}
			else if (type[0] == 'A')
			{
				chanacs_t *ca;
				char *chan = strtok(name, ":");
				char *mask = strtok(NULL, " ");

				ca = chanacs_find_by_mask(mychan_find(chan), mask, CA_NONE);
				if (ca != NULL)
					metadata_add(ca, METADATA_CHANACS, property, value);
			}
		}

		/* Channel URLs */
		if (!strcmp("UR", item))
		{
			char *chan, *url;

			chan = strtok(NULL, " ");
			url = strtok(NULL, " ");

			strip(url);

			if (chan && url)
			{
				mc = mychan_find(chan);

				if (mc)
					metadata_add(mc, METADATA_CHANNEL, "url", url);
			}
		}

		/* Channel entry messages */
		if (!strcmp("EM", item))
		{
			char *chan, *message;

			chan = strtok(NULL, " ");
			message = strtok(NULL, "");

			strip(message);

			if (chan && message)
			{
				mc = mychan_find(chan);

				if (mc)
					metadata_add(mc, METADATA_CHANNEL, "private:entrymsg", message);
			}
		}

		/* chanacs */
		if (!strcmp("CA", item))
		{
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
					slog(LG_ERROR, "db_load(): invalid chanacs (line %d)",linecnt);
					continue;
				}

				cain++;

				if (i >= DB_ATHEME)
				{
					uint32_t fl = flags_to_bitmask(strtok(NULL, " "), chanacs_flags, 0x0);

					/* Compatibility with oldworld Atheme db's. --nenolod */
					/* arbitrary cutoff to avoid touching newer +voOt entries -- jilles */
					if (fl == OLD_CA_AOP && i < 4)
						fl = CA_AOP_DEF;

					/* previous to CA_ACLVIEW, everyone could view
					 * access lists. If they aren't AKICKed, upgrade
					 * them. This keeps us from breaking old XOPs.
					 */
					if (i < 4)
						if (!(fl & CA_AKICK))
							fl |= CA_ACLVIEW;

					if ((!mu) && (validhostmask(causer)))
						ca = chanacs_add_host(mc, causer, fl);
					else
						ca = chanacs_add(mc, mu, fl);
				}
				else if (i == DB_SHRIKE)	/* DB_SHRIKE */
				{
					uint32_t fl = atol(strtok(NULL, " "));
					uint32_t fl2 = 0x0;

					switch (fl)
					{
					  case SHRIKE_CA_VOP:
						  fl2 = chansvs.ca_vop;
					  case SHRIKE_CA_AOP:
						  fl2 = chansvs.ca_hop;
					  case SHRIKE_CA_SOP:
						  fl2 = chansvs.ca_sop;
					  case SHRIKE_CA_SUCCESSOR:
						  fl2 = CA_SUCCESSOR_0;
					  case SHRIKE_CA_FOUNDER:
						  fl2 = CA_FOUNDER_0;
					}

					if ((!mu) && (validhostmask(causer)))
						ca = chanacs_add_host(mc, causer, fl2);
					else
						ca = chanacs_add(mc, mu, fl2);
				}
			}
		}


		/* Services ignores */
		if (!strcmp("SI", item))
		{
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

		/* klines */
		if (!strcmp("KL", item))
		{
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

			k = kline_add(user, host, reason, duration);
			k->settime = settime;
			/* XXX this is not nice, oh well -- jilles */
			k->expires = k->settime + k->duration;
			k->setby = sstrdup(setby);

			kin++;
		}

		/* end */
		if (!strcmp("DE", item))
		{
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
		}
	}

	fclose(f);

	slog(LG_DEBUG, "db_load(): ------------------------- done -------------------------");
}

void _modinit(module_t *m)
{
	m->mflags = MODTYPE_CORE;

	db_load = &flatfile_db_load;
	db_save = &flatfile_db_save;

	backend_loaded = TRUE;
}
