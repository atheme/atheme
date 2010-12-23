/* XML RPC Library
 *
 * (C) 2005 Trystan Scott Lee
 * Contact trystan@nomadirc.net
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code from Denora
 * 
 *
 */

#include <mowgli.h>
#include "atheme.h"
#include "xmlrpclib.h"

static int xmlrpc_error_code;

typedef struct XMLRPCCmd_ XMLRPCCmd;

struct XMLRPCCmd_ {
	XMLRPCMethodFunc func;
	char *name;
	int core;
	char *mod_name;
	XMLRPCCmd *next;
};

mowgli_patricia_t *XMLRPCCMD = NULL;

struct xmlrpc_settings {
	char *(*setbuffer)(char *buffer, int len);
	char *encode;
	int httpheader;
	char *inttagstart;
	char *inttagend;
} xmlrpc;

static char *xmlrpc_parse(char *buffer);
static char *xmlrpc_method(char *buffer);
static int xmlrpc_split_buf(char *buffer, char ***argv);
static void xmlrpc_append_char_encode(string_t *s, const char *s1);

static XMLRPCCmd *createXMLCommand(const char *name, XMLRPCMethodFunc func);
static int addXMLCommand(XMLRPCCmd * xml);
static char *xmlrpc_write_header(int length);

/*************************************************************************/

int xmlrpc_getlast_error(void)
{
	return xmlrpc_error_code;
}

/*************************************************************************/

void xmlrpc_process(char *buffer, void *userdata)
{
	int retVal = 0;
	XMLRPCCmd *current = NULL;
	XMLRPCCmd *xml;
	char *tmp;
	int ac;
	char **av = NULL;
	char *name = NULL;

	xmlrpc_error_code = 0;

	if (!buffer)
	{
		xmlrpc_error_code = -1;
		return;
	}

	tmp = xmlrpc_parse(buffer);
	if (tmp)
	{
		name = xmlrpc_method(tmp);
		if (name)
		{
			xml = mowgli_patricia_retrieve(XMLRPCCMD, name);
			if (xml)
			{
				ac = xmlrpc_split_buf(tmp, &av);
				if (xml->func)
				{
					retVal = xml->func(userdata, ac, av);
					if (retVal == XMLRPC_CONT)
					{
						current = xml->next;
						while (current && current->func && retVal == XMLRPC_CONT)
						{
							retVal = current->func(userdata, ac, av);
							current = current->next;
						}
					}
					else
					{	/* we assume that XMLRPC_STOP means the handler has given no output */
						xmlrpc_error_code = -7;
						xmlrpc_generic_error(xmlrpc_error_code, "XMLRPC error: First eligible function returned XMLRPC_STOP");
					}
				}
				else
				{
					xmlrpc_error_code = -6;
					xmlrpc_generic_error(xmlrpc_error_code, "XMLRPC error: Method has no registered function");
				}
			}
			else
			{
				xmlrpc_error_code = -4;
				xmlrpc_generic_error(xmlrpc_error_code, "XMLRPC error: Unknown routine called");
			}
		}
		else
		{
			xmlrpc_error_code = -3;
			xmlrpc_generic_error(xmlrpc_error_code, "XMLRPC error: Missing methodRequest or methodName.");
		}
	}
	else
	{
		xmlrpc_error_code = -2;
		xmlrpc_generic_error(xmlrpc_error_code, "XMLRPC error: Invalid document end at line 1");
	}
	free(av);
	free(tmp);
	free(name);
}

/*************************************************************************/

void xmlrpc_set_buffer(char *(*func) (char *buffer, int len))
{
	return_if_fail(func != NULL);
	xmlrpc.setbuffer = func;
}

/*************************************************************************/

int xmlrpc_register_method(const char *name, XMLRPCMethodFunc func)
{
	XMLRPCCmd *xml;

	return_val_if_fail(name != NULL, XMLRPC_ERR_PARAMS);
	return_val_if_fail(func != NULL, XMLRPC_ERR_PARAMS);
	xml = createXMLCommand(name, func);

	return addXMLCommand(xml);
}

/*************************************************************************/

static XMLRPCCmd *createXMLCommand(const char *name, XMLRPCMethodFunc func)
{
	XMLRPCCmd *xml = NULL;
	xml = smalloc(sizeof(XMLRPCCmd));
	xml->name = sstrdup(name);
	xml->func = func;
	xml->mod_name = NULL;
	xml->core = 0;
	xml->next = NULL;
	return xml;
}

/*************************************************************************/

static int addXMLCommand(XMLRPCCmd * xml)
{
	if (XMLRPCCMD == NULL)
		XMLRPCCMD = mowgli_patricia_create(strcasecanon);

	mowgli_patricia_add(XMLRPCCMD, xml->name, xml);

	return XMLRPC_ERR_OK;
}

/*************************************************************************/

int xmlrpc_unregister_method(const char *method)
{
	return_val_if_fail(method != NULL, XMLRPC_ERR_PARAMS);

	mowgli_patricia_delete(XMLRPCCMD, method);

	return XMLRPC_ERR_OK;
}

/*************************************************************************/

static char *xmlrpc_write_header(int length)
{
	char buf[512];
	time_t ts;
	char timebuf[64];
	struct tm tm;

	*buf = '\0';

	ts = time(NULL);
	tm = *localtime(&ts);
	strftime(timebuf, sizeof timebuf, "%Y-%m-%d %H:%M:%S", &tm);

	snprintf(buf, sizeof buf, "HTTP/1.1 200 OK\r\nConnection: close\r\n" "Content-Length: %d\r\n" "Content-Type: text/xml\r\n" "Date: %s\r\n" "Server: Atheme/%s\r\n\r\n", length, timebuf, PACKAGE_VERSION);
	return sstrdup(buf);
}

/*************************************************************************/

/**
 * Parse Data down to just the <xml> without any \r\n or \t
 * @param buffer Incoming data buffer
 * @return cleaned up buffer
 */
static char *xmlrpc_parse(char *buffer)
{
	char *tmp = NULL;

	/*
	   Okay since the buffer could contain 
	   HTTP header information, lets break
	   off at the point that the <?xml?> starts
	 */
	tmp = strstr(buffer, "<?xml");

	/* check if its xml doc */
	if (tmp)
	{
		/* get all the odd characters out of the data */
		return xmlrpc_normalizeBuffer(tmp);
	}
	return NULL;
}

/*************************************************************************/

/* Splits up a series of values into an array of strings.
 * The given buffer is modified and the pointers in the array point to
 * parts of the buffer. The array itself is dynamically allocated.
 * Returns the number of values placed in the array.
 * Largely rewritten by jilles 20080803
 */
static int xmlrpc_split_buf(char *buffer, char ***argv)
{
	int ac = 0;
	int argvsize = 8;
	char *data, *str;
	char *nexttag = NULL;
	char *p;
	int tagtype = 0;

	data = buffer;
	*argv = smalloc(sizeof(char *) * argvsize);
	while ((data = strstr(data, "<value>")))
	{
		data += 7;
		nexttag = strchr(data, '<');
		if (nexttag == NULL)
			break;
		nexttag++;
		p = strchr(nexttag, '>');
		if (p == NULL)
			break;
		*p++ = '\0';
		/* strings */
		if (!stricmp("string", nexttag))
			tagtype = 1;
		else
			tagtype = 0;
		str = p;
		p = strchr(str, '<');
		if (p == NULL)
			break;
		*p++ = '\0';
		if (ac >= argvsize)
		{
			argvsize *= 2;
			*argv = srealloc(*argv, sizeof(char *) * argvsize);
		}
		if (tagtype == 1)
			(*argv)[ac++] = xmlrpc_decode_string(str);
		else
			(*argv)[ac++] = str;
		data = p;
	}			/* while */
	return ac;
}

/*************************************************************************/

static char *xmlrpc_method(char *buffer)
{
	char *data, *p, *name;
	int namelen;

	data = strstr(buffer, "<methodName>");
	if (data)
	{
		data += 12;
		p = strchr(data, '<');
		if (p == NULL)
			return NULL;
		namelen = p - data;
		name = smalloc(namelen + 1);
		memcpy(name, data, namelen);
		name[namelen] = '\0';
		return name;
	}
	return NULL;
}

/*************************************************************************/

void xmlrpc_generic_error(int code, const char *string)
{
	char buf[1024];
	const char *ss;
	string_t *s = new_string(XMLRPC_BUFSIZE);
	char *s2;
	int len;

	if (xmlrpc.encode)
	{
		snprintf(buf, sizeof buf, "<?xml version=\"1.0\" encoding=\"%s\" ?>\r\n<methodResponse>\r\n", xmlrpc.encode);
	}
	else
	{
		snprintf(buf, sizeof buf, "<?xml version=\"1.0\"?>\r\n<methodResponse>\r\n");
	}
	s->append(s, buf, strlen(buf));

	ss = " <fault>\r\n  <value>\r\n   <struct>\r\n    <member>\r\n     <name>faultCode</name>\r\n     <value><int>";
	s->append(s, ss, strlen(ss));
	snprintf(buf, sizeof buf, "%d", code);
	s->append(s, buf, strlen(buf));
	ss = "</int></value>\r\n    </member>\r\n    <member>\r\n     <name>faultString</name>\r\n     <value><string>";
	s->append(s, ss, strlen(ss));
	xmlrpc_append_char_encode(s, string);
	ss = "</string></value>\r\n    </member>\r\n   </struct>\r\n  </value>\r\n </fault>\r\n</methodResponse>",
	s->append(s, ss, strlen(ss));

	len = s->pos;

	if (xmlrpc.httpheader)
	{
		char *header = xmlrpc_write_header(len);
		s2 = smalloc(strlen(header) + len + 1);
		strcpy(s2, header);
		memcpy(s2 + strlen(header), s->str, len);
		xmlrpc.setbuffer(s2, len + strlen(header));
		free(header);
		free(s2);
	}
	else
		xmlrpc.setbuffer(s->str, len);
	s->delete(s);
}

/*************************************************************************/

int xmlrpc_about(void *userdata, int ac, char **av)
{
	char buf[XMLRPC_BUFSIZE];
	char buf2[XMLRPC_BUFSIZE];
	char buf3[XMLRPC_BUFSIZE];
	char buf4[XMLRPC_BUFSIZE];
	char *arraydata;

	(void)userdata;
	xmlrpc_integer(buf3, ac);
	xmlrpc_string(buf4, av[0]);
	xmlrpc_string(buf, (char *)XMLLIB_VERSION);
	xmlrpc_string(buf2, (char *)XMLLIB_AUTHOR);
	arraydata = xmlrpc_array(4, buf, buf2, buf3, buf4);

	xmlrpc_send(1, arraydata);
	free(arraydata);
	return XMLRPC_CONT;
}

/*************************************************************************/

void xmlrpc_send(int argc, ...)
{
	va_list va;
	int idx = 0;
	int len;
	char buf[1024];
	const char *ss;
	string_t *s = new_string(XMLRPC_BUFSIZE);
	char *s2;
	char *header;

	if (xmlrpc.encode)
	{
		snprintf(buf, sizeof buf, "<?xml version=\"1.0\" encoding=\"%s\" ?>\r\n<methodResponse>\r\n<params>\r\n", xmlrpc.encode);
	}
	else
	{
		snprintf(buf, sizeof buf, "<?xml version=\"1.0\"?>\r\n<methodResponse>\r\n<params>\r\n");
	}
	s->append(s, buf, strlen(buf));

	va_start(va, argc);
	for (idx = 0; idx < argc; idx++)
	{
		ss = " <param>\r\n  <value>\r\n   ";
		s->append(s, ss, strlen(ss));
		ss = va_arg(va, const char *);
		s->append(s, ss, strlen(ss));
		ss = "\r\n  </value>\r\n </param>\r\n";
		s->append(s, ss, strlen(ss));
	}
	va_end(va);

	ss = "</params>\r\n</methodResponse>";
	s->append(s, ss, strlen(ss));

	len = s->pos;

	if (xmlrpc.httpheader)
	{
		header = xmlrpc_write_header(len);
		s2 = smalloc(strlen(header) + len + 1);
		strcpy(s2, header);
		memcpy(s2 + strlen(header), s->str, len);
		xmlrpc.setbuffer(s2, len + strlen(header));
		free(header);
		free(s2);
		xmlrpc.httpheader = 1;
	}
	else
	{
		xmlrpc.setbuffer(s->str, len);
	}
	if (xmlrpc.encode)
	{
		free(xmlrpc.encode);
		xmlrpc.encode = NULL;
	}
	s->delete(s);
}

/*************************************************************************/

void xmlrpc_send_string(const char *value)
{
	int len;
	char buf[1024];
	const char *ss;
	string_t *s = new_string(XMLRPC_BUFSIZE);
	char *s2;
	char *header;

	if (xmlrpc.encode)
	{
		snprintf(buf, sizeof buf, "<?xml version=\"1.0\" encoding=\"%s\" ?>\r\n<methodResponse>\r\n<params>\r\n", xmlrpc.encode);
	}
	else
	{
		snprintf(buf, sizeof buf, "<?xml version=\"1.0\"?>\r\n<methodResponse>\r\n<params>\r\n");
	}
	s->append(s, buf, strlen(buf));

	ss = " <param>\r\n  <value>\r\n   <string>";
	s->append(s, ss, strlen(ss));
	xmlrpc_append_char_encode(s, value);
	ss = "</string>\r\n  </value>\r\n </param>\r\n";
	s->append(s, ss, strlen(ss));

	ss = "</params>\r\n</methodResponse>";
	s->append(s, ss, strlen(ss));

	len = s->pos;

	if (xmlrpc.httpheader)
	{
		header = xmlrpc_write_header(len);
		s2 = smalloc(strlen(header) + len + 1);
		strcpy(s2, header);
		memcpy(s2 + strlen(header), s->str, len);
		xmlrpc.setbuffer(s2, len + strlen(header));
		free(header);
		free(s2);
		xmlrpc.httpheader = 1;
	}
	else
	{
		xmlrpc.setbuffer(s->str, len);
	}
	if (xmlrpc.encode)
	{
		free(xmlrpc.encode);
		xmlrpc.encode = NULL;
	}
	s->delete(s);
}

/*************************************************************************/

char *xmlrpc_time2date(char *buf, time_t t)
{
	char timebuf[XMLRPC_BUFSIZE];
	struct tm *tm;

	*buf = '\0';
	tm = localtime(&t);
	/* <dateTime.iso8601>20011003T08:53:38</dateTime.iso8601> */
	strftime(timebuf, XMLRPC_BUFSIZE - 1, "%Y%m%dT%I:%M:%S", tm);
	snprintf(buf, XMLRPC_BUFSIZE, "<dateTime.iso8601>%s</dateTime.iso8601>", timebuf);
	return buf;
}

/*************************************************************************/

char *xmlrpc_integer(char *buf, int value)
{
	*buf = '\0';

	if (!xmlrpc.inttagstart || !xmlrpc.inttagend)
	{
		snprintf(buf, XMLRPC_BUFSIZE, "<i4>%d</i4>", value);
	}
	else
	{
		snprintf(buf, XMLRPC_BUFSIZE, "%s%d%s", xmlrpc.inttagstart, value, xmlrpc.inttagend);
		free(xmlrpc.inttagstart);
		if (xmlrpc.inttagend)
		{
			free(xmlrpc.inttagend);
			xmlrpc.inttagend = NULL;
		}
		xmlrpc.inttagstart = NULL;
	}
	return buf;
}

/*************************************************************************/

char *xmlrpc_string(char *buf, const char *value)
{
	char encoded[XMLRPC_BUFSIZE];
	*buf = '\0';

	xmlrpc_char_encode(encoded, value);
	snprintf(buf, XMLRPC_BUFSIZE, "<string>%s</string>", encoded);
	return buf;
}

/*************************************************************************/

char *xmlrpc_boolean(char *buf, int value)
{
	*buf = '\0';
	snprintf(buf, XMLRPC_BUFSIZE, "<boolean>%d</boolean>", (value ? 1 : 0));
	return buf;
}

/*************************************************************************/

char *xmlrpc_double(char *buf, double value)
{
	*buf = '\0';
	snprintf(buf, XMLRPC_BUFSIZE, "<double>%g</double>", value);
	return buf;
}

/*************************************************************************/

char *xmlrpc_array(int argc, ...)
{
	va_list va;
	char *a;
	int idx = 0;
	char *s = NULL;
	int len;
	char buf[XMLRPC_BUFSIZE];

	va_start(va, argc);
	for (idx = 0; idx < argc; idx++)
	{
		a = va_arg(va, char *);
		if (!s)
		{
			snprintf(buf, XMLRPC_BUFSIZE, "   <value>%s</value>", a);
			s = sstrdup(buf);
		}
		else
		{
			snprintf(buf, XMLRPC_BUFSIZE, "%s\r\n     <value>%s</value>", s, a);
			free(s);
			s = sstrdup(buf);
		}
	}
	va_end(va);

	snprintf(buf, XMLRPC_BUFSIZE, "<array>\r\n    <data>\r\n  %s\r\n    </data>\r\n   </array>", s);
	len = strlen(buf);
	free(s);
	return sstrdup(buf);
}

/*************************************************************************/

char *xmlrpc_normalizeBuffer(const char *buf)
{
	char *newbuf;
	int i, len, j = 0;

	len = strlen(buf);
	newbuf = (char *)smalloc(sizeof(char) * len + 1);

	for (i = 0; i < len; i++)
	{
		switch (buf[i])
		{
			  /* ctrl char */
		  case 1:
			  break;
			  /* Bold ctrl char */
		  case 2:
			  break;
			  /* Color ctrl char */
		  case 3:
			  /* If the next character is a digit, its also removed */
			  if (isdigit(buf[i + 1]))
			  {
				  i++;

				  /* not the best way to remove colors
				   * which are two digit but no worse then
				   * how the Unreal does with +S - TSL
				   */
				  if (isdigit(buf[i + 1]))
				  {
					  i++;
				  }

				  /* Check for background color code
				   * and remove it as well
				   */
				  if (buf[i + 1] == ',')
				  {
					  i++;

					  if (isdigit(buf[i + 1]))
					  {
						  i++;
					  }
					  /* not the best way to remove colors
					   * which are two digit but no worse then
					   * how the Unreal does with +S - TSL
					   */
					  if (isdigit(buf[i + 1]))
					  {
						  i++;
					  }
				  }
			  }

			  break;
			  /* tabs char */
		  case 9:
			  break;
			  /* line feed char */
		  case 10:
			  break;
			  /* carrage returns char */
		  case 13:
			  break;
			  /* Reverse ctrl char */
		  case 22:
			  break;
			  /* Underline ctrl char */
		  case 31:
			  break;
			  /* A valid char gets copied into the new buffer */
		  default:
			  /* All valid <32 characters are handled above. */
			  if (buf[i] > 31)
			  {
				newbuf[j] = buf[i];
				j++;
			  }
		}
	}

	/* Terminate the string */
	newbuf[j] = 0;

	return (newbuf);
}

/*************************************************************************/

int xmlrpc_set_options(int type, const char *value)
{
	if (type == XMLRPC_HTTP_HEADER)
	{
		if (!stricmp(value, XMLRPC_ON))
		{
			xmlrpc.httpheader = 1;
		}
		if (!stricmp(value, XMLRPC_OFF))
		{
			xmlrpc.httpheader = 0;
		}
	}
	if (type == XMLRPC_ENCODE)
	{
		if (value)
		{
			xmlrpc.encode = sstrdup(value);
		}
	}
	if (type == XMLRPC_INTTAG)
	{
		if (!stricmp(value, XMLRPC_I4))
		{
			xmlrpc.inttagstart = sstrdup("<i4>");
			xmlrpc.inttagend = sstrdup("</i4>");
		}
		if (!stricmp(value, XMLRPC_INT))
		{
			xmlrpc.inttagstart = sstrdup("<int>");
			xmlrpc.inttagend = sstrdup("</int>");
		}
	}
	return 1;
}

/*************************************************************************/

void xmlrpc_char_encode(char *outbuffer, const char *s1)
{
	long unsigned int i;
	unsigned char c;
	char buf2[15];
	string_t *s = new_string(XMLRPC_BUFSIZE);
	*buf2 = '\0';
	*outbuffer = '\0';

	if ((!(s1) || (*(s1) == '\0')))
	{
		return;
	}

	for (i = 0; s1[i] != '\0'; i++)
	{
		c = s1[i];
		if (c > 127)
		{
			snprintf(buf2, sizeof buf2, "&#%d;", c);
			s->append(s, buf2, strlen(buf2));
		}
		else if (c == '&')
		{
			s->append(s, "&amp;", 5);
		}
		else if (c == '<')
		{
			s->append(s, "&lt;", 4);
		}
		else if (c == '>')
		{
			s->append(s, "&gt;", 4);
		}
		else if (c == '"')
		{
			s->append(s, "&quot;", 6);
		}
		else
		{
			s->append_char(s, c);
		}
	}

	memcpy(outbuffer, s->str, XMLRPC_BUFSIZE);
}

static void xmlrpc_append_char_encode(string_t *s, const char *s1)
{
	long unsigned int i;
	unsigned char c;
	char buf2[15];

	if ((!(s1) || (*(s1) == '\0')))
	{
		return;
	}

	for (i = 0; s1[i] != '\0'; i++)
	{
		c = s1[i];
		if (c > 127)
		{
			snprintf(buf2, sizeof buf2, "&#%d;", c);
			s->append(s, buf2, strlen(buf2));
		}
		else if (c == '&')
		{
			s->append(s, "&amp;", 5);
		}
		else if (c == '<')
		{
			s->append(s, "&lt;", 4);
		}
		else if (c == '>')
		{
			s->append(s, "&gt;", 4);
		}
		else if (c == '"')
		{
			s->append(s, "&quot;", 6);
		}
		else
		{
			s->append_char(s, c);
		}
	}
}

/* In-place decode of some entities
 * rewritten by jilles 20080802
 */
char *xmlrpc_decode_string(char *buf)
{
	const char *p;
	char *q;

	p = buf;
	q = buf;
	while (*p != '\0')
	{
		if (*p == '&')
		{
			p++;
			if (!strncmp(p, "gt;", 3))
				*q++ = '>', p += 3;
			else if (!strncmp(p, "lt;", 3))
				*q++ = '<', p += 3;
			else if (!strncmp(p, "quot;", 5))
				*q++ = '"', p += 5;
			else if (!strncmp(p, "amp;", 4))
				*q++ = '&', p += 4;
			else if (*p == '#')
			{
				p++;
				*q++ = (char)atoi(p);
				while (*p != ';' && *p != '\0')
					p++;
			}
		}
		else
			*q++ = *p++;
	}
	*q = '\0';

	return buf;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
