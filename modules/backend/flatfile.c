/*
 * SPDX-License-Identifier: ISC
 * SPDX-URL: https://spdx.org/licenses/ISC.html
 *
 * Copyright (C) 2005-2007 Atheme Project (http://atheme.org/)
 *
 * This file contains the implementation of the Atheme 0.1
 * flatfile database format, with metadata extensions.
 */

#include <atheme.h>

// database versions
#define DB_SHRIKE	1
#define DB_ATHEME	2

// loads atheme.db
static void ATHEME_FATTR_NORETURN
flatfile_db_load(const char *filename)
{
	struct myuser *mu, *founder = NULL;
	struct myuser_name *mun;
	struct mychan *mc;
	struct kline *k;
	struct xline *x;
	struct qline *q;
	struct svsignore *svsignore;
	unsigned int versn = 0, i;
	unsigned int linecnt = 0, muin = 0, mcin = 0, cain = 0, kin = 0, xin = 0, qin = 0;
	FILE *f;
	char *item, *s, *buf;
	size_t bufsize = BUFSIZE, n;
	int c;
	unsigned int their_ca_all = 0;
	char path[BUFSIZE];

	snprintf(path, BUFSIZE, "%s/%s", datadir, filename != NULL ? filename : "atheme.db");
	f = fopen(path, "r");
	if (f == NULL)
	{
		if (errno == ENOENT)
		{
			slog(LG_ERROR, "db_load(): %s does not exist.  as no data will be converted, you should use the 'opensex' backend now.", path);
			exit(EXIT_FAILURE);
		}
		else
		{
			slog(LG_ERROR, "db_load(): can't open %s for reading: %s", path, strerror(errno));
			slog(LG_ERROR, "db_load(): exiting to avoid data loss");
			exit(EXIT_FAILURE);
		}
	}

	slog(LG_DEBUG, "db_load(): ----------------------- loading ------------------------");

	buf = smalloc(bufsize);

	// start reading it, one line at a time
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
			slog(LG_ERROR, "db_load(): error while reading %s: %s", path, strerror(errno));
			slog(LG_ERROR, "db_load(): exiting to avoid data loss");
			exit(EXIT_FAILURE);
		}

		if (c == EOF && n == 0)
			break;

		linecnt++;

		// check for unimportant lines
		item = strtok(buf, " ");
		strip(item);

		if (item == NULL || *item == '#' || *item == '\n' || *item == '\t' || *item == ' ' || *item == '\0' || *item == '\r')
			continue;

		// database version
		if (!strcmp("DBV", item))
		{
			versn = atoi(strtok(NULL, " "));
			if (versn > 6)
			{
				slog(LG_INFO, "db_load(): database version is %u; i only understand 5 (Atheme 2.0, 2.1), "
					"4 (Atheme 0.2), 3 (Atheme 0.2 without CA_ACLVIEW), 2 (Atheme 0.1) or 1 (Shrike)", versn);
				exit(EXIT_FAILURE);
			}
		}
		else if (!strcmp("CF", item))
		{
			// enabled chanacs flags
			s = strtok(NULL, " ");
			if (s == NULL)
				slog(LG_INFO, "db_load(): missing param to CF");
			else
			{
				their_ca_all = flags_to_bitmask(s, 0);
				if (their_ca_all & ~ca_all)
				{
					slog(LG_ERROR, "db_load(): losing flags %s from file", bitmask_to_flags(their_ca_all & ~ca_all));
				}
				if (~their_ca_all & ca_all)
				{
					slog(LG_ERROR, "db_load(): making up flags %s not present in file", bitmask_to_flags(~their_ca_all & ca_all));
				}
			}
		}
		else if (!strcmp("MU", item))
		{
			// myusers
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
					slog(LG_INFO, "db_load(): skipping duplicate account %s (line %u)", s, linecnt);
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
			// memo
			char *sender, *text;
			time_t mtime;
			unsigned int status;
			struct mymemo *mz;

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

			mz = smalloc(sizeof *mz);

			mowgli_strlcpy(mz->sender, sender, sizeof mz->sender);
			mowgli_strlcpy(mz->text, text, sizeof mz->text);
			mz->sent = mtime;
			mz->status = status;

			if (!(mz->status & MEMO_READ))
				mu->memoct_new++;

			mowgli_node_add(mz, mowgli_node_create(), &mu->memos);
		}
		else if (!strcmp("MI", item))
		{
			// memo ignore
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

			mowgli_node_add(strbuf, mowgli_node_create(), &mu->memo_ignores);
		}
		else if (!strcmp("AC", item))
		{
			// myuser access list
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
			// registered nick
			char *user, *nick, *treg, *tseen;
			struct mynick *mn;

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
				slog(LG_INFO, "db_load(): skipping duplicate nick %s (account %s) (line %u)", nick, user, linecnt);
				continue;
			}

			mn = mynick_add(mu, nick);
			mn->registered = atoi(treg);
			mn->lastseen = atoi(tseen);
		}
		else if (!strcmp("MCFP", item))
		{
			// certfp
			char *user, *certfp;
			struct mycertfp *mcfp;

			user = strtok(NULL, " ");
			certfp = strtok(NULL, " ");

			mu = myuser_find(user);
			if (mu == NULL || certfp == NULL)
			{
				slog(LG_DEBUG, "db_load(): invalid certfp<%s> for user<%s>", certfp, user);
				continue;
			}

			mcfp = mycertfp_add(mu, certfp, true);
		}
		else if (!strcmp("NAM", item))
		{
			// formerly registered name (created by a marked account being dropped)
			char *user;

			user = strtok(NULL, " \n");
			if (!user)
			{
				slog(LG_INFO, "db_load(): invalid old name (line %u)", linecnt);
				continue;
			}
			myuser_name_add(user);
		}
		else if (!strcmp("SO", item))
		{
			// services oper
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
			soper_add(entity(mu)->name, class, atoi(flagstr) & ~SOPER_CONF, password);
		}
		else if (!strcmp("MC", item))
		{
			// mychans
			char *mcname;

			if ((s = strtok(NULL, " ")))
			{
				if (mychan_find(s))
				{
					slog(LG_INFO, "db_load(): skipping duplicate channel %s (line %u)", s, linecnt);
					continue;
				}

				mcin++;

				mcname = s;

				// unused (old password)
				(void) strtok(NULL, " ");

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
			// Metadata entry
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
				struct chanacs *ca;
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
			// Channel URLs (obsolete)
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
			// Channel entry messages (obsolete)
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
			// chanacs
			struct chanacs *ca;
			char *cachan, *causer;

			cachan = strtok(NULL, " ");
			causer = strtok(NULL, " ");

			if (cachan && causer)
			{
				mc = mychan_find(cachan);
				mu = myuser_find(causer);

				if (mc == NULL || (mu == NULL && !validhostmask(causer)))
				{
					slog(LG_ERROR, "db_load(): invalid chanacs (line %u)", linecnt);
					continue;
				}

				cain++;

				if (versn >= DB_ATHEME)
				{
					unsigned int fl = flags_to_bitmask(strtok(NULL, " "), 0);
					const char *tsstr;
					time_t ts = 0;

					// Compatibility with oldworld Atheme db's. --nenolod
					// arbitrary cutoff to avoid touching newer +voOt entries -- jilles
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
						slog(LG_INFO, "db_load(): old database, making up flags %s", bitmask_to_flags(~their_ca_all & ca_all));
					}

					/* Grant +h if they have +o,
					 * the file does not have +h enabled
					 * and we currently have +h enabled.
					 * This preserves AOP, SOP and +*.
					 */
					if (fl & CA_OP && !(their_ca_all & CA_HALFOP) && ca_all & CA_HALFOP)
						fl |= CA_HALFOP;

					// Set new-style founder flag
					if (founder != NULL && mu == founder && !(their_ca_all & CA_FOUNDER))
						fl |= CA_FOUNDER;

					// Set new-style +q and +a flags
					if (fl & CA_SET && fl & CA_OP && !(their_ca_all & CA_USEPROTECT) && ca_all & CA_USEPROTECT)
						fl |= CA_USEPROTECT;
					if (fl & CA_FOUNDER && !(their_ca_all & CA_USEOWNER) && ca_all & CA_USEOWNER)
						fl |= CA_USEOWNER;

					if ((!mu) && (validhostmask(causer)))
						ca = chanacs_add_host(mc, causer, fl, ts, NULL);
					else
						ca = chanacs_add(mc, entity(mu), fl, ts, NULL);
				}
				else if (versn == DB_SHRIKE)	// DB_SHRIKE
				{
					unsigned int fl = atol(strtok(NULL, " "));
					unsigned int fl2 = 0x0;
					time_t ts = CURRTIME;

					switch (fl)
					{
					  case SHRIKE_CA_VOP:
						  fl2 = get_template_flags(mc, "VOP");
						  break;
					  case SHRIKE_CA_AOP:
						  fl2 = get_template_flags(mc, "AOP");
						  break;
					  case SHRIKE_CA_SOP:
						  fl2 = get_template_flags(mc, "SOP");
						  break;
					  case SHRIKE_CA_SUCCESSOR:
						  fl2 = CA_SUCCESSOR_0;
						  break;
					  case SHRIKE_CA_FOUNDER:
						  fl2 = CA_FOUNDER_0;
						  break;
					}

					if ((!mu) && (validhostmask(causer)))
						ca = chanacs_add_host(mc, causer, fl2, ts, NULL);
					else
						ca = chanacs_add(mc, entity(mu), fl2, ts, NULL);
				}
			}
		}
		else if (!strcmp("SI", item))
		{
			// Services ignores
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
			// unique kline id
			char *id = strtok(NULL, " ");
			me.kline_id = atol(id);
		}
		else if (!strcmp("KL", item))
		{
			// klines
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

			// XXX this is not nice, oh well -- jilles
			k->expires = k->settime + k->duration;

			kin++;
		}
		else if (!strcmp("XID", item))
		{
			// unique xline id
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

			// XXX this is not nice, oh well -- jilles
			x->expires = x->settime + x->duration;

			xin++;
		}
		else if (!strcmp("QID", item))
		{
			// unique qline id
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

			// XXX this is not nice, oh well -- jilles
			q->expires = q->settime + q->duration;

			qin++;
		}
		else if (!strcmp("DE", item))
		{
			// end
			i = atoi(strtok(NULL, " "));
			if (i != muin)
				slog(LG_ERROR, "db_load(): got %u myusers; expected %u", muin, i);

			i = atoi(strtok(NULL, " "));
			if (i != mcin)
				slog(LG_ERROR, "db_load(): got %u mychans; expected %u", mcin, i);

			i = atoi(strtok(NULL, " "));
			if (i != cain)
				slog(LG_ERROR, "db_load(): got %u chanacs; expected %u", cain, i);

			if ((s = strtok(NULL, " ")))
				if ((i = atoi(s)) != kin)
					slog(LG_ERROR, "db_load(): got %u klines; expected %u", kin, i);

			if ((s = strtok(NULL, " ")))
				if ((i = atoi(s)) != xin)
					slog(LG_ERROR, "db_load(): got %u xlines; expected %u", xin, i);

			if ((s = strtok(NULL, " ")))
				if ((i = atoi(s)) != qin)
					slog(LG_ERROR, "db_load(): got %u qlines; expected %u", qin, i);
		}
	}

	fclose(f);

	sfree(buf);

	slog(LG_DEBUG, "db_load(): ------------------------- done -------------------------");
	db_save(NULL, DB_SAVE_BLOCKING);

	slog(LG_INFO, "Your database has been converted to the new OpenSEX format automatically.");
	slog(LG_INFO, "You must now change the backend module in the config file to ensure that the OpenSEX database is loaded.");
	slog(LG_INFO, "Exiting...");

	exit(EXIT_SUCCESS);
}

static void
mod_init(struct module *const restrict m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "backend/opensex")

	db_load = &flatfile_db_load;

	backend_loaded = true;
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{

}

SIMPLE_DECLARE_MODULE_V1("backend/flatfile", MODULE_UNLOAD_CAPABILITY_NEVER)
