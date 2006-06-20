/*
 * Copyright (c) 2005 Atheme Development Group
 *
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains config file parsing routines.
 * This code was taken from Sentinel: copyright W. Campbell.
 *
 * $Id: confparse.c 5468 2006-06-20 23:31:25Z jilles $
 */

#include "atheme.h"

static void config_error(char *format, ...);
static CONFIGFILE *config_parse(char *filename, char *confdata);
static void config_entry_free(CONFIGENTRY *ceptr);

static void config_error(char *format, ...)
{
	va_list ap;
	char buffer[1024];
	char *ptr;

	va_start(ap, format);
	vsnprintf(buffer, 1024, format, ap);
	va_end(ap);
	if ((ptr = strchr(buffer, '\n')) != NULL)
		*ptr = '\0';
	slog(LG_ERROR, "config_parse(): %s", buffer);
}

static CONFIGFILE *config_parse(char *filename, char *confdata)
{
	char *ptr;
	char *start;
	int linenumber = 1;
	CONFIGENTRY *curce;
	CONFIGENTRY **lastce;
	CONFIGENTRY *cursection;

	CONFIGFILE *curcf;
	CONFIGFILE *lastcf;

	lastcf = curcf = (CONFIGFILE *)smalloc(sizeof(CONFIGFILE));
	memset(curcf, 0, sizeof(CONFIGFILE));
	curcf->cf_filename = sstrdup(filename);
	lastce = &(curcf->cf_entries);
	curce = NULL;
	cursection = NULL;

	for (ptr = confdata; *ptr; ptr++)
	{
		switch (*ptr)
		{
		  case '#':
			  while (*++ptr && (*ptr != '\n'))
				  ;
			  if (!*ptr)
			  {
				  /* make for(;;) exit from the loop */
				  ptr--;
				  continue;
			  }
			  linenumber++;
			  break;
		  case ';':
			  if (!curce)
			  {
				  config_error("%s:%i Ignoring extra semicolon\n", filename, linenumber);
				  break;
			  }
			  if (!strcmp(curce->ce_varname, "include"))
			  {
				  CONFIGFILE *cfptr;

				  if (!curce->ce_vardata)
				  {
					  config_error("%s:%i Ignoring \"include\": No filename given\n", filename, linenumber);
					  config_entry_free(curce);
					  curce = NULL;
					  continue;
				  }
				  if (strlen(curce->ce_vardata) > 255)
					  curce->ce_vardata[255] = '\0';
				  cfptr = config_load(curce->ce_vardata);
				  if (cfptr)
				  {
					  lastcf->cf_next = cfptr;
					  lastcf = cfptr;
				  }
				  config_entry_free(curce);
				  curce = NULL;
				  continue;
			  }
			  *lastce = curce;
			  lastce = &(curce->ce_next);
			  curce->ce_fileposend = (ptr - confdata);
			  curce = NULL;
			  break;
		  case '{':
			  if (!curce)
			  {
				  config_error("%s:%i: No name for section start\n", filename, linenumber);
				  continue;
			  }
			  else if (curce->ce_entries)
			  {
				  config_error("%s:%i: Ignoring extra section start\n", filename, linenumber);
				  continue;
			  }
			  curce->ce_sectlinenum = linenumber;
			  lastce = &(curce->ce_entries);
			  cursection = curce;
			  curce = NULL;
			  break;
		  case '}':
			  if (curce)
			  {
				  config_error("%s:%i: Missing semicolon before close brace\n", filename, linenumber);
				  config_entry_free(curce);
				  config_free(curcf);
				  return NULL;
			  }
			  else if (!cursection)
			  {
				  config_error("%s:%i: Ignoring extra close brace\n", filename, linenumber);
				  continue;
			  }
			  curce = cursection;
			  cursection->ce_fileposend = (ptr - confdata);
			  cursection = cursection->ce_prevlevel;
			  if (!cursection)
				  lastce = &(curcf->cf_entries);
			  else
				  lastce = &(cursection->ce_entries);
			  for (; *lastce; lastce = &((*lastce)->ce_next))
				  continue;
			  break;
		  case '/':
			  if (*(ptr + 1) == '/')
			  {
				  ptr += 2;
				  while (*ptr && (*ptr != '\n'))
					  ptr++;
				  if (!*ptr)
					  break;
				  ptr--;	/* grab the \n on next loop thru */
				  continue;
			  }
			  else if (*(ptr + 1) == '*')
			  {
				  int commentstart = linenumber;

				  for (ptr += 2; *ptr; ptr++)
				  {
					  if ((*ptr == '*') && (*(ptr + 1) == '/'))
					  {
						  ptr++;
						  break;
					  }
					  else if (*ptr == '\n')
						  linenumber++;
				  }
				  if (!*ptr)
				  {
					  config_error("%s:%i Comment on this line does not end\n", filename, commentstart);
					  config_entry_free(curce);
					  config_free(curcf);
					  return NULL;
				  }
			  }
			  break;
		  case '\"':
			  start = ++ptr;
			  for (; *ptr; ptr++)
			  {
				  if ((*ptr == '\\') && (*(ptr + 1) == '\"'))
				  {
					  char *tptr = ptr;
					  while ((*tptr = *(tptr + 1)))
						  tptr++;
				  }
				  else if ((*ptr == '\"') || (*ptr == '\n'))
					  break;
			  }
			  if (!*ptr || (*ptr == '\n'))
			  {
				  config_error("%s:%i: Unterminated quote found\n", filename, linenumber);
				  config_entry_free(curce);
				  config_free(curcf);
				  return NULL;
			  }
			  if (curce)
			  {
				  if (curce->ce_vardata)
				  {
					  config_error("%s:%i: Ignoring extra data\n", filename, linenumber);
				  }
				  else
				  {
					  char *eptr;

					  curce->ce_vardata = (char *)smalloc(ptr - start + 1);
					  strlcpy(curce->ce_vardata, start, ptr - start + 1);
					  curce->ce_vardatanum = strtol(curce->ce_vardata, &eptr, 0) & 0xffffffff;	/* we only want 32bits and long is 64bit on 64bit compiles */
					  if (eptr != (curce->ce_vardata + (ptr - start)))
					  {
						  curce->ce_vardatanum = 0;
					  }
				  }
			  }
			  else
			  {
				  curce = (CONFIGENTRY *)smalloc(sizeof(CONFIGENTRY));
				  memset(curce, 0, sizeof(CONFIGENTRY));
				  curce->ce_varname = (char *)smalloc(ptr - start + 1);
				  strlcpy(curce->ce_varname, start, ptr - start + 1);
				  curce->ce_varlinenum = linenumber;
				  curce->ce_fileptr = curcf;
				  curce->ce_prevlevel = cursection;
				  curce->ce_fileposstart = (start - confdata);
			  }
			  break;
		  case '\n':
			  linenumber++;
			  break;
		  case '\t':
		  case ' ':
		  case '=':
		  case '\r':
			  break;
		  default:
			  if ((*ptr == '*') && (*(ptr + 1) == '/'))
			  {
				  config_error("%s:%i Ignoring extra end comment\n", filename, linenumber);
				  ptr++;
				  break;
			  }
			  start = ptr;
			  for (; *ptr; ptr++)
			  {
				  if ((*ptr == ' ') || (*ptr == '\t') || (*ptr == '\n') || (*ptr == ';'))
					  break;
			  }
			  if (!*ptr)
			  {
				  if (curce)
					  config_error("%s: Unexpected EOF for variable starting at %i\n", filename, curce->ce_varlinenum);
				  else if (cursection)
					  config_error("%s: Unexpected EOF for section starting at %i\n", filename, curce->ce_sectlinenum);
				  else
					  config_error("%s: Unexpected EOF.\n", filename);
				  config_entry_free(curce);
				  config_free(curcf);
				  return NULL;
			  }
			  if (curce)
			  {
				  if (curce->ce_vardata)
				  {
					  config_error("%s:%i: Ignoring extra data\n", filename, linenumber);
				  }
				  else
				  {
					  char *eptr;

					  curce->ce_vardata = (char *)smalloc(ptr - start + 1);
					  strlcpy(curce->ce_vardata, start, ptr - start + 1);
					  curce->ce_vardatanum = strtol(curce->ce_vardata, &eptr, 0) & 0xffffffff;	/* we only want 32bits and long is 64bit on 64bit compiles */
					  if (eptr != (curce->ce_vardata + (ptr - start)))
					  {
						  curce->ce_vardatanum = 0;
					  }
				  }
			  }
			  else
			  {
				  curce = (CONFIGENTRY *)smalloc(sizeof(CONFIGENTRY));
				  memset(curce, 0, sizeof(CONFIGENTRY));
				  curce->ce_varname = (char *)smalloc(ptr - start + 1);
				  strlcpy(curce->ce_varname, start, ptr - start + 1);
				  curce->ce_varlinenum = linenumber;
				  curce->ce_fileptr = curcf;
				  curce->ce_prevlevel = cursection;
				  curce->ce_fileposstart = (start - confdata);
			  }
			  if ((*ptr == ';') || (*ptr == '\n'))
				  ptr--;
			  break;
		}		/* switch */
	}			/* for */
	if (curce)
	{
		config_error("%s: Unexpected EOF for variable starting on line %i\n", filename, curce->ce_varlinenum);
		config_entry_free(curce);
		config_free(curcf);
		return NULL;
	}
	else if (cursection)
	{
		config_error("%s: Unexpected EOF for section starting on line %i\n", filename, cursection->ce_sectlinenum);
		config_free(curcf);
		return NULL;
	}
	return curcf;
}

static void config_entry_free(CONFIGENTRY *ceptr)
{
	CONFIGENTRY *nptr;

	for (; ceptr; ceptr = nptr)
	{
		nptr = ceptr->ce_next;
		if (ceptr->ce_entries)
			config_entry_free(ceptr->ce_entries);
		if (ceptr->ce_varname)
			free(ceptr->ce_varname);
		if (ceptr->ce_vardata)
			free(ceptr->ce_vardata);
		free(ceptr);
	}
}

void config_free(CONFIGFILE *cfptr)
{
	CONFIGFILE *nptr;

	for (; cfptr; cfptr = nptr)
	{
		nptr = cfptr->cf_next;
		if (cfptr->cf_entries)
			config_entry_free(cfptr->cf_entries);
		if (cfptr->cf_filename)
			free(cfptr->cf_filename);
		free(cfptr);
	}
}

CONFIGFILE *config_load(char *filename)
{
	struct stat sb;
	FILE *fd;
	int ret;
	char *buf = NULL;
	CONFIGFILE *cfptr;

	fd = fopen(filename, "rb");
	if (!fd)
	{
		config_error("Couldn't open \"%s\": %s\n", filename, strerror(errno));
		return NULL;
	}
	if (stat(filename, &sb) == -1)
	{
		config_error("Couldn't fstat \"%s\": %s\n", filename, strerror(errno));
		fclose(fd);
		return NULL;
	}
	if (!sb.st_size)
	{
		fclose(fd);
		return NULL;
	}
	buf = (char *)smalloc(sb.st_size + 1);
	if (buf == NULL)
	{
		config_error("Out of memory trying to load \"%s\"\n", filename);
		fclose(fd);
		return NULL;
	}
	ret = fread(buf, 1, sb.st_size, fd);
	if (ret != sb.st_size)
	{
		config_error("Error reading \"%s\": %s\n", filename, ret == -1 ? strerror(errno) : strerror(EFAULT));
		free(buf);
		fclose(fd);
		return NULL;
	}
	buf[ret] = '\0';
	fclose(fd);
	cfptr = config_parse(filename, buf);
	free(buf);
	return cfptr;
}

CONFIGENTRY *config_find(CONFIGENTRY *ceptr, char *name)
{
	for (; ceptr; ceptr = ceptr->ce_next)
		if (!strcmp(ceptr->ce_varname, name))
			break;
	return ceptr;
}
