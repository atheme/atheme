/*
 * atheme-services: A collection of minimalist IRC services   
 * confparse.c: Initialization and startup of the services system
 *
 * Copyright (c) 2005-2007 Atheme Project (http://www.atheme.org)           
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Description of config files parsed by this:
 *
 * configfile   = *WS *configentry
 * configentry  = value [1*WS value] [1*WS "{" *(configentry 1*WS) "}" ] *WS ";"
 * value        = 1*achar / DQUOTE *qchar DQUOTE
 * achar        = <any CHAR except WS or DQUOTE>
 * qchar        = <any CHAR except DQUOTE or \> / "\\" / "\" DQUOTE
 * comment      = "/" "*" <anything except * /> "*" "/" /
 *                "#" *CHAR %0x0A /
 *                "//" *CHAR %0x0A
 * WS           = %x09 / %x0A / %x0D / SPACE / "=" / comment
 *
 * A value of "include" for toplevel configentries causes a file to be
 * included. The included file is logically appended to the current file,
 * no matter where the include directive is. Include files must have balanced
 * braces.
 */
/*
 * Original idea from the csircd config parser written by Fred Jacobs
 * and Chris Behrens.
 */

#include "atheme.h"
#include <sys/stat.h>
#include <limits.h>

#define MAX_INCLUDE_NESTING 16

static config_file_t *config_parse(const char *filename, char *confdata);
static void config_entry_free(config_entry_t *ceptr);
static config_file_t *config_load_internal(config_file_t *parent, const char *filename);

#define CF_ERRORED(cf) ((cf)->cf_curline <= 0)

static void config_error(config_file_t *cf, const char *format, ...)
{
	va_list ap;
	char buffer[1024];
	char *ptr;

	va_start(ap, format);
	vsnprintf(buffer, 1024, format, ap);
	va_end(ap);
	if ((ptr = strchr(buffer, '\n')) != NULL)
		*ptr = '\0';
	if (cf != NULL)
	{
		if (cf->cf_curline < 0)
			cf->cf_curline = -cf->cf_curline;
		slog(LG_ERROR, "%s:%d: %s",
				cf->cf_filename, cf->cf_curline, buffer);
		/* mark config parse as failed */
		cf->cf_curline = -cf->cf_curline;
	}
	else
		slog(LG_ERROR, "config_parse(): %s",
				buffer);
}

static void skip_ws(char ** restrict pos, config_file_t * restrict cf)
{
	int startline;

	for (;;)
	{
		switch (**pos)
		{
			case ' ':
			case '\t':
			case '\r':
			case '=': /* XXX */
				break;
			case '\n':
				cf->cf_curline++;
				break;
			case '/':
				if ((*pos)[1] == '*')
				{
					startline = cf->cf_curline;
					(*pos)++;
					(*pos)++;
					while (**pos != '\0' && (**pos != '*' || (*pos)[1] != '/'))
					{
						if (**pos == '\n')
							cf->cf_curline++;
						(*pos)++;
					}
					if (**pos == '\0')
						config_error(cf, "File ends inside comment starting at line %d", startline);
					else
						(*pos)++; /* skip '*' */
				}
				else if ((*pos)[1] == '/')
				{
					while (**pos != '\0' && **pos != '\n' && **pos != '\r')
						(*pos)++;
					continue;
				}
				else
					return;
				break;
			case '#':
				while (**pos != '\0' && **pos != '\n' && **pos != '\r')
					(*pos)++;
				continue;
			default:
				return;
		}
		if (**pos == '\0')
			return;
		(*pos)++;
	}
}

static char *get_value(char **pos, config_file_t *cf, char * restrict skipped)
{
	char *p = *pos;
	char *q;
	char *start;

	*skipped = '\0';
	if (*p == '"')
	{
		p++;
		start = p;
		q = p;
		while (*p != '\0' && *p != '\r' && *p != '\n' && *p != '"')
		{
			if (*p == '\\' && (p[1] == '"' || p[1] == '\\'))
				p++;
			*q++ = *p++;
		}
		if (*p == '\0')
		{
			config_error(cf, "File ends inside quoted string");
			return NULL;
		}
		if (*p == '\r' || *p == '\n')
		{
			config_error(cf, "Newline inside quoted string");
			return NULL;
		}
		if (*p != '"')
		{
			config_error(cf, "Weird character terminating quoted string (BUG)");
			return NULL;
		}
		p++;
		*q = '\0';
		*pos = p;
		skip_ws(pos, cf);
		return start;
	}
	else
	{
		start = p;
		while (*p != '\0' && *p != '\t' && *p != '\r' && *p != '\n' &&
				*p != ' ' && *p != '/' && *p != '#' &&
				*p != ';' && *p != '{' && *p != '}')
			p++;
		if (p == start)
			return NULL;
		*pos = p;
		skip_ws(pos, cf);
		if (p == *pos)
			*skipped = *p;
		*p = '\0';
		if (p == *pos)
			(*pos)++;
		return start;
	}
}

static config_file_t *config_parse(const char *filename, char *confdata)
{
	config_file_t *cf, *subcf, *lastcf;
	config_entry_t **pprevce, *ce, *upce;
	char *p, *val;
	char c;

	cf = smalloc(sizeof *cf);
	cf->cf_filename = sstrdup(filename);
	cf->cf_curline = 1;
	cf->cf_mem = confdata;
	lastcf = cf;
	pprevce = &cf->cf_entries;
	upce = NULL;
	p = confdata;
	while (*p != '\0')
	{
		skip_ws(&p, cf);
		if (*p == '\0' || CF_ERRORED(cf))
			break;
		if (*p == '}')
		{
			if (upce == NULL)
			{
				config_error(cf, "Extraneous closing brace");
				break;
			}
			ce = upce;
			ce->ce_sectlinenum = cf->cf_curline;
			pprevce = &ce->ce_next;
			upce = ce->ce_prevlevel;
			p++;
			skip_ws(&p, cf);
			if (CF_ERRORED(cf))
				break;
			if (*p != ';')
			{
				config_error(cf, "Missing semicolon after closing brace for section ending at line %d", ce->ce_sectlinenum);
				break;
			}
			ce = NULL;
			p++;
			continue;
		}
		val = get_value(&p, cf, &c);
		if (CF_ERRORED(cf))
			break;
		if (val == NULL)
		{
			config_error(cf, "Unexpected character trying to read variable name");
			break;
		}
		ce = smalloc(sizeof *ce);
		ce->ce_fileptr = cf;
		ce->ce_varlinenum = cf->cf_curline;
		ce->ce_varname = val;
		ce->ce_prevlevel = upce;
		*pprevce = ce;
		pprevce = &ce->ce_next;
		if (c == '\0' && (*p == '{' || *p == ';'))
			c = *p++;
		if (c == '{')
		{
			pprevce = &ce->ce_entries;
			upce = ce;
			ce = NULL;
		}
		else if (c == ';')
		{
			ce = NULL;
		}
		else if (c != '\0')
		{
			config_error(cf, "Unexpected characters after unquoted string %s", ce->ce_varname);
			break;
		}
		else
		{
			val = get_value(&p, cf, &c);
			if (CF_ERRORED(cf))
				break;
			if (val == NULL)
			{
				config_error(cf, "Unexpected character trying to read value for %s", ce->ce_varname);
				break;
			}
			ce->ce_vardata = val;
			if (c == '\0' && (*p == '{' || *p == ';'))
				c = *p++;
			if (c == '{')
			{
				pprevce = &ce->ce_entries;
				upce = ce;
				ce = NULL;
			}
			else if (c == ';')
			{
				if (upce == NULL && !strcasecmp(ce->ce_varname, "include"))
				{
					subcf = config_load_internal(cf, ce->ce_vardata);
					if (subcf == NULL)
					{
						config_error(cf, "Error in file included from here");
						break;
					}
					lastcf->cf_next = subcf;
					while (lastcf->cf_next != NULL)
						lastcf = lastcf->cf_next;
				}
				ce = NULL;
			}
			else
			{
				config_error(cf, "Unexpected characters after value %s %s", ce->ce_varname, ce->ce_vardata);
				break;
			}
		}
	}
	if (!CF_ERRORED(cf) && upce != NULL)
	{
		config_error(cf, "One or more sections not closed");
		ce = upce;
		while (ce->ce_prevlevel != NULL)
			ce = ce->ce_prevlevel;
		if (ce->ce_vardata != NULL)
			config_error(cf, "First unclosed section is %s %s at line %d",
					ce->ce_varname, ce->ce_vardata, ce->ce_varlinenum);
		else
			config_error(cf, "First unclosed section is %s at line %d",
					ce->ce_varname, ce->ce_varlinenum);
	}
	if (CF_ERRORED(cf))
	{
		config_free(cf);
		cf = NULL;
	}
	return cf;
}

static void config_entry_free(config_entry_t *ceptr)
{
	config_entry_t *nptr;

	for (; ceptr; ceptr = nptr)
	{
		nptr = ceptr->ce_next;
		if (ceptr->ce_entries)
			config_entry_free(ceptr->ce_entries);
		/* ce_varname and ce_vardata are inside cf_mem */
		free(ceptr);
	}
}

void config_free(config_file_t *cfptr)
{
	config_file_t *nptr;

	for (; cfptr; cfptr = nptr)
	{
		nptr = cfptr->cf_next;
		if (cfptr->cf_entries)
			config_entry_free(cfptr->cf_entries);
		free(cfptr->cf_filename);
		free(cfptr->cf_mem);
		free(cfptr);
	}
}

static config_file_t *config_load_internal(config_file_t *parent, const char *filename)
{
	struct stat sb;
	FILE *fp;
	size_t ret;
	char *buf = NULL;
	config_file_t *cfptr;
	static int nestcnt;

	if (nestcnt > MAX_INCLUDE_NESTING)
	{
		config_error(parent, "Includes nested too deep \"%s\"\n", filename);
		return NULL;
	}

	fp = fopen(filename, "rb");
	if (!fp)
	{
		config_error(parent, "Couldn't open \"%s\": %s\n", filename, strerror(errno));
		return NULL;
	}
	if (stat(filename, &sb) == -1)
	{
		config_error(parent, "Couldn't fstat \"%s\": %s\n", filename, strerror(errno));
		fclose(fp);
		return NULL;
	}
	if (!S_ISREG(sb.st_mode))
	{
		config_error(parent, "Not a regular file: \"%s\"\n", filename);
		fclose(fp);
		return NULL;
	}
	if (sb.st_size > SSIZE_MAX - 1)
	{
		config_error(parent, "File too large: \"%s\"\n", filename);
		fclose(fp);
		return NULL;
	}
	buf = (char *)smalloc(sb.st_size + 1);
	if (sb.st_size)
	{
		errno = 0;
		ret = fread(buf, 1, sb.st_size, fp);
		if (ret != (size_t)sb.st_size)
		{
			config_error(parent, "Error reading \"%s\": %s\n", filename, strerror(errno ? errno : EFAULT));
			free(buf);
			fclose(fp);
			return NULL;
		}
	}
	else
		ret = 0;
	buf[ret] = '\0';
	fclose(fp);
	nestcnt++;
	cfptr = config_parse(filename, buf);
	nestcnt--;
	/* buf is owned by cfptr or freed now */
	return cfptr;
}

config_file_t *config_load(const char *filename)
{
	return config_load_internal(NULL, filename);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
