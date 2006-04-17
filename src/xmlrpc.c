/* XML RPC Library
 *
 * (C) 2005 Trystan Scott Lee
 * Contact trystan@nomadirc.net
 *
 * Please read COPYING and README for furhter details.
 *
 * Based on the original code from Denora
 * 
 *
 */

#include "xmlrpc.h"

int xmlrpc_error_code;

XMLRPCCmdHash *XMLRPCCMD[MAX_CMD_HASH];

XMLRPCSet xmlrpc;

static int xmlrpc_tolower(char c);
static char *xmlrpc_GetToken(const char *str, const char dilim, int token_number);
static char *xmlrpc_GetTokenRemainder(const char *str, const char dilim, int token_number);
static char *xmlrpc_myStrSubString(const char *src, int start, int end);
static char *xmlrpc_strdup(const char *src);
static char *xmlrpc_parse(char *buffer);
static char *xmlrpc_method(char *buffer);
static char *xmlrpc_normalizeBuffer(char *buf);
static char *xmlrpc_stristr(char *s1, char *s2);
static int xmlrpc_split_buf(char *buffer, char ***argv);

XMLRPCCmd *createXMLCommand(const char *name, XMLRPCMethodFunc func);
XMLRPCCmd *findXMLRPCCommand(XMLRPCCmdHash * hookEvtTable[], const char *name);
int addXMLCommand(XMLRPCCmdHash * hookEvtTable[], XMLRPCCmd * xml);
int destroyXMLRPCCommand(XMLRPCCmd * xml);
int delXMLRPCCommand(XMLRPCCmdHash * msgEvtTable[], XMLRPCCmd * xml, char *mod_name);
char *xmlrpc_write_header(int length);
int addCoreXMLRPCCmd(XMLRPCCmdHash * hookEvtTable[], XMLRPCCmd * xml);
XMLRPCCmd *first_xmlrpccmd(void);
XMLRPCCmd *next_xmlrpccmd(void);
XMLRPCCmdHash *first_xmlrpchash(void);
XMLRPCCmdHash *next_xmlrpchash(void);
int destroyxmlrpchash(XMLRPCCmdHash * mh);
char *xmlrcp_strnrepl(char *s, int32_t size, const char *old, const char *new);
int xmlrpc_myNumToken(const char *str, const char dilim);

/*************************************************************************/

int xmlrpc_getlast_error(void)
{
	if (!xmlrpc_error_code)
	{
		return 0;
	}
	else
	{
		return xmlrpc_error_code;
	}
}

/*************************************************************************/

void xmlrpc_process(char *buffer, void *userdata)
{
	int retVal = 0;
	XMLRPCCmd *current = NULL;
	XMLRPCCmd *xml;
	char *tmp;
	int ac = 0;
	char **av;
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
			xml = findXMLRPCCommand(XMLRPCCMD, name);
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
	if (ac)
	{
		free(av);
	}
	if (tmp)
	{
		free(tmp);
	}
	if (name)
	{
		free(name);
	}
}

/*************************************************************************/

void xmlrpc_set_buffer(char *(*func) (char *buffer, int len))
{
	xmlrpc.setbuffer = NULL;
	if (func)
	{
		xmlrpc.setbuffer = func;
	}
	else
	{
		xmlrpc_error_code = -8;
	}
}

/*************************************************************************/

int xmlrpc_register_method(const char *name, XMLRPCMethodFunc func)
{
	XMLRPCCmd *xml;
	xml = createXMLCommand(name, func);
	return addXMLCommand(XMLRPCCMD, xml);
}

/*************************************************************************/

XMLRPCCmd *createXMLCommand(const char *name, XMLRPCMethodFunc func)
{
	XMLRPCCmd *xml = NULL;
	if (!func)
	{
		return NULL;
	}
	if ((xml = malloc(sizeof(XMLRPCCmd))) == NULL)
	{
		return NULL;
	}
	xml->name = xmlrpc_strdup(name);
	xml->func = func;
	xml->next = NULL;
	return xml;
}

/*************************************************************************/

XMLRPCCmd *findXMLRPCCommand(XMLRPCCmdHash * hookEvtTable[], const char *name)
{
	int idx;
	XMLRPCCmdHash *current = NULL;
	if (!hookEvtTable || !name)
	{
		return NULL;
	}
	idx = CMD_HASH(name);

	for (current = hookEvtTable[idx]; current; current = current->next)
	{
		if (stricmp(name, current->name) == 0)
		{
			return current->xml;
		}
	}
	return NULL;
}

/*************************************************************************/

int addXMLCommand(XMLRPCCmdHash * hookEvtTable[], XMLRPCCmd * xml)
{
	/* We can assume both param's have been checked by this point.. */
	int idx = 0;
	XMLRPCCmdHash *current = NULL;
	XMLRPCCmdHash *newHash = NULL;
	XMLRPCCmdHash *lastHash = NULL;

	if (!hookEvtTable || !xml)
	{
		return XMLRPC_ERR_PARAMS;
	}
	idx = CMD_HASH(xml->name);

	for (current = hookEvtTable[idx]; current; current = current->next)
	{
		if (stricmp(xml->name, current->name) == 0)
		{
			xml->next = current->xml;
			current->xml = xml;
			return XMLRPC_ERR_OK;
		}
		lastHash = current;
	}

	if ((newHash = malloc(sizeof(XMLRPCCmdHash))) == NULL)
	{
		return XMLRPC_ERR_OK;
	}
	newHash->next = NULL;
	newHash->name = xmlrpc_strdup(xml->name);
	newHash->xml = xml;

	if (lastHash == NULL)
		hookEvtTable[idx] = newHash;
	else
		lastHash->next = newHash;
	return XMLRPC_ERR_OK;
}

/*************************************************************************/

int xmlrpc_unregister_method(const char *method)
{
	XMLRPCCmd *xml;

	xml = findXMLRPCCommand(XMLRPCCMD, method);
	return destroyXMLRPCCommand(xml);
}

/*************************************************************************/

/**
 * Destory a message, freeing its memory.
 * @param m the message to be destroyed
 * @return XMLRPC_ERR_SUCCESS on success
 **/
int destroyXMLRPCCommand(XMLRPCCmd * xml)
{
	if (!xml)
	{
		return XMLRPC_ERR_PARAMS;
	}
	if (xml->name)
	{
		free(xml->name);
	}
	xml->func = NULL;
	if (xml->mod_name)
	{
		free(xml->mod_name);
	}
	xml->next = NULL;
	return XMLRPC_ERR_OK;
}

/*************************************************************************/

int delXMLRPCCommand(XMLRPCCmdHash * msgEvtTable[], XMLRPCCmd * xml, char *mod_name)
{
	int idx = 0;
	XMLRPCCmdHash *current = NULL;
	XMLRPCCmdHash *lastHash = NULL;
	XMLRPCCmd *tail = NULL, *last = NULL;

	if (!xml || !msgEvtTable)
	{
		return XMLRPC_ERR_PARAMS;
	}
	idx = CMD_HASH(xml->name);

	for (current = msgEvtTable[idx]; current; current = current->next)
	{
		if (stricmp(xml->name, current->name) == 0)
		{
			if (!lastHash)
			{
				tail = current->xml;
				if (tail->next)
				{
					while (tail)
					{
						if (mod_name && tail->mod_name && (stricmp(mod_name, tail->mod_name) == 0))
						{
							if (last)
							{
								last->next = tail->next;
							}
							else
							{
								current->xml = tail->next;
							}
							return XMLRPC_ERR_OK;
						}
						last = tail;
						tail = tail->next;
					}
				}
				else
				{
					msgEvtTable[idx] = current->next;
					free(current->name);
					return XMLRPC_ERR_OK;
				}
			}
			else
			{
				tail = current->xml;
				if (tail->next)
				{
					while (tail)
					{
						if (mod_name && tail->mod_name && (stricmp(mod_name, tail->mod_name) == 0))
						{
							if (last)
							{
								last->next = tail->next;
							}
							else
							{
								current->xml = tail->next;
							}
							return XMLRPC_ERR_OK;
						}
						last = tail;
						tail = tail->next;
					}
				}
				else
				{
					lastHash->next = current->next;
					free(current->name);
					return XMLRPC_ERR_OK;
				}
			}
		}
		lastHash = current;
	}
	return XMLRPC_ERR_NOEXIST;
}

/*************************************************************************/

int addCoreXMLRPCCmd(XMLRPCCmdHash * hookEvtTable[], XMLRPCCmd * xml)
{
	if (!hookEvtTable || !xml)
	{
		return XMLRPC_ERR_PARAMS;
	}
	xml->core = 1;
	return addXMLCommand(hookEvtTable, xml);
}

/*************************************************************************/

char *xmlrpc_write_header(int length)
{
	char buf[XMLRPC_BUFSIZE];
	time_t ts;
	char timebuf[64];
	struct tm tm;

	*buf = '\0';

	ts = time(NULL);
	tm = *localtime(&ts);
	strftime(timebuf, XMLRPC_BUFSIZE - 1, "%Y-%m-%d %H:%M:%S", &tm);

	snprintf(buf, XMLRPC_BUFSIZE, "HTTP/1.1 200 OK\n\rConnection: close\n\r" "Content-Length: %d\n\r" "Content-Type: text/xml\n\r" "Date: %s\n\r" "Server: Atheme %s\r\n\r\n", length, timebuf, version);
	return xmlrpc_strdup(buf);
}

/*************************************************************************/

/**
 * Parse Data down to just the <xml> without any \r\n or \t
 * @param buffer Incoming data buffer
 * @return cleaned up buffer
 */
char *xmlrpc_parse(char *buffer)
{
	char *tmp = NULL;

	/*
	   Okay since the buffer could contain 
	   HTTP header information, lets break
	   off at the point that the <?xml?> starts
	 */
	tmp = xmlrpc_stristr(buffer, (char *)"<?xml");

	/* check if its xml doc */
	if (tmp)
	{
		/* get all the odd characters out of the data */
		return xmlrpc_normalizeBuffer(tmp);
	}
	return NULL;
}

/*************************************************************************/

static int xmlrpc_split_buf(char *buffer, char ***argv)
{
	int ac = 0;
	int argvsize = 8;
	char *data, *str;
	char *str2 = NULL;
	char *nexttag = NULL;
	char *temp = NULL;
	char *final;
	int tagtype = 0;

	*argv = calloc(sizeof(char *) * argvsize, 1);
	while ((data = strstr(buffer, "<value>")))
	{
		if (data)
		{
			temp = xmlrpc_GetToken(data, '<', 2);
			nexttag = xmlrpc_GetToken(temp, '>', 0);
			/* strings */
			if (!stricmp("string", nexttag))
			{
				tagtype = 1;
			}
			else
			{
				tagtype = 0;
			}
			str = xmlrpc_GetToken(data, '>', 2);
			str2 = xmlrpc_GetTokenRemainder(data, '>', 2);
			if (str)
			{
				final = xmlrpc_GetToken(str, '<', 0);
				if (final)
				{
					if (tagtype == 1)
					{
						(*argv)[ac++] = xmlrpc_decode_string(final);
					}
					else
					{
						(*argv)[ac++] = final;
					}
				}
				free(str);
			}	/* str */
		}		/* data */
		buffer = str2;
	}			/* while */
	free(str2);
	return ac;
}

/*************************************************************************/

char *xmlrpc_method(char *buffer)
{
	char *data;
	char *tmp, *tmp2;

	data = xmlrpc_stristr(buffer, (char *)"<methodname>");
	if (data)
	{
		tmp = xmlrpc_GetToken(data, '>', 1);
		tmp2 = xmlrpc_GetToken(tmp, '<', 0);
		free(tmp);
		return tmp2;
	}
	return NULL;
}

/*************************************************************************/

void xmlrpc_generic_error(int code, const char *string)
{
	char buf[XMLRPC_BUFSIZE], buf2[XMLRPC_BUFSIZE];
	int len;

	snprintf(buf, XMLRPC_BUFSIZE,
		 "<?xml version=\"1.0\"?>\r\n <methodResponse>\n\r  <fault>\n\r   <value>\n\r    <struct>\n\r     <member>\n\r      <name>faultCode</name>\n\r      <value><int>%d</int></value>\n\r     </member>\n\r     <member>\n\r      <name>faultString</name>\n\r      <value><string>%s</string></value>\n\r     </member>\n\r    </struct>\n\r   </value>\r\n  </fault>\r\n </methodResponse>",
		 code, string);
	len = strlen(buf);

	if (xmlrpc.httpheader)
	{
		char *header = xmlrpc_write_header(len);
		strlcpy(buf2, header, XMLRPC_BUFSIZE);
		strlcat(buf2, buf, XMLRPC_BUFSIZE);
		len += strlen(header);
		free(header);
		xmlrpc.setbuffer(buf2, len);
	}
	else
		xmlrpc.setbuffer(buf, len);
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
	char *a;
	int idx = 0;
	char *s = NULL;
	int len;
	char buf[XMLRPC_BUFSIZE];
	char buf2[XMLRPC_BUFSIZE];
	char *header;

	*buf = '\0';
	*buf2 = '\0';

	va_start(va, argc);
	for (idx = 0; idx < argc; idx++)
	{
		a = va_arg(va, char *);
		if (!s)
		{
			snprintf(buf, XMLRPC_BUFSIZE, " <param>\r\n  <value>\r\n   %s\r\n  </value>\r\n </param>", a);
			s = xmlrpc_strdup(buf);
		}
		else
		{
			snprintf(buf, XMLRPC_BUFSIZE, "%s\r\n <param>\r\n  <value>\r\n   %s\r\n  </value>\r\n </param>", s, a);
			free(s);
			s = xmlrpc_strdup(buf);
		}
	}
	va_end(va);

	if (xmlrpc.encode)
	{
		snprintf(buf, XMLRPC_BUFSIZE, "<?xml version=\"1.0\" encoding=\"%s\" ?>\r\n<methodResponse>\r\n<params>\r\n%s\r\n</params>\r\n</methodResponse>", xmlrpc.encode, s);
	}
	else
	{
		snprintf(buf, XMLRPC_BUFSIZE, "<?xml version=\"1.0\"?>\r\n<methodResponse>\r\n<params>\r\n%s\r\n</params>\r\n</methodResponse>", s);
	}
	len = strlen(buf);

	if (xmlrpc.httpheader)
	{
		header = xmlrpc_write_header(len);
		snprintf(buf2, XMLRPC_BUFSIZE, "%s%s", header, buf);
		free(header);
		len = strlen(buf2);
		xmlrpc.setbuffer(buf2, len);
		xmlrpc.httpheader = 1;
	}
	else
	{
		xmlrpc.setbuffer(buf, len);
	}
	if (xmlrpc.encode)
	{
		free(xmlrpc.encode);
		xmlrpc.encode = NULL;
	}
	free(s);
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

char *xmlrpc_string(char *buf, char *value)
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
			s = xmlrpc_strdup(buf);
		}
		else
		{
			snprintf(buf, XMLRPC_BUFSIZE, "%s\r\n     <value>%s</value>", s, a);
			free(s);
			s = xmlrpc_strdup(buf);
		}
	}
	va_end(va);

	snprintf(buf, XMLRPC_BUFSIZE, "<array>\r\n    <data>\r\n  %s\r\n    </data>\r\n   </array>", s);
	len = strlen(buf);
	free(s);
	return xmlrpc_strdup(buf);
}

/*************************************************************************/

static XMLRPCCmdHash *current;
static int next_index;

XMLRPCCmd *first_xmlrpccmd(void)
{
	next_index = 0;

	while (next_index < 1024 && current == NULL)
		current = XMLRPCCMD[next_index++];
	if (current)
	{
		return current->xml;
	}
	else
	{
		return NULL;
	}
}

/*************************************************************************/

XMLRPCCmd *next_xmlrpccmd(void)
{
	if (current)
		current = current->next;
	if (!current && next_index < 1024)
	{
		while (next_index < 1024 && current == NULL)
			current = XMLRPCCMD[next_index++];
	}
	if (current)
	{
		return current->xml;
	}
	else
	{
		return NULL;
	}
}

/*************************************************************************/

XMLRPCCmdHash *first_xmlrpchash(void)
{
	next_index = 0;

	while (next_index < 1024 && current == NULL)
		current = XMLRPCCMD[next_index++];
	return current;
}

/*************************************************************************/

XMLRPCCmdHash *next_xmlrpchash(void)
{
	if (current)
		current = current->next;
	if (!current && next_index < 1024)
	{
		while (next_index < 1024 && current == NULL)
			current = XMLRPCCMD[next_index++];
	}
	if (current)
	{
		return current;
	}
	else
	{
		return NULL;
	}
}

/*************************************************************************/

int destroyxmlrpchash(XMLRPCCmdHash * mh)
{
	if (!mh)
	{
		return XMLRPC_ERR_PARAMS;
	}
	if (mh->name)
	{
		free(mh->name);
	}
	mh->xml = NULL;
	mh->next = NULL;
	free(mh);
	return XMLRPC_ERR_OK;
}

/*************************************************************************/

static char *xmlrpc_strdup(const char *src)
{
	char *ret = NULL;

	if (src)
	{
#ifdef __STRICT_ANSI__
		if ((ret = (char *)malloc(strlen(src) + 1)))
		{
			strcpy(ret, src);
		}
#else
		if ((ret = (char *)malloc(strlen(src) + 1)))
		{
			strcpy(ret, src);
		}
#endif
		if (!ret)
#ifndef _WIN32
			raise(SIGUSR1);
#else
			abort();
#endif
	}
	return ret;
}

/*************************************************************************/

/**
 * Get the token
 * @param str String to search in
 * @param dilim Character to search for
 * @param token_number the token number
 * @return token
 */
static char *xmlrpc_GetToken(const char *str, const char dilim, int token_number)
{
	int len, idx, counter = 0, start_pos = 0;
	char *substring = NULL;
	if (!str)
	{
		return NULL;
	}

	len = strlen(str);
	for (idx = 0; idx <= len; idx++)
	{
		if ((str[idx] == dilim) || (idx == len))
		{
			if (counter == token_number)
			{
				substring = xmlrpc_myStrSubString(str, start_pos, idx);
				counter++;
			}
			else
			{
				start_pos = idx + 1;
				counter++;
			}
		}
	}
	return substring;
}

/*************************************************************************/

/**
 * Get the Remaining tokens
 * @param str String to search in
 * @param dilim Character to search for
 * @param token_number the token number
 * @return token
 */
static char *xmlrpc_GetTokenRemainder(const char *str, const char dilim, int token_number)
{
	int len, idx, counter = 0, start_pos = 0;
	char *substring = NULL;
	if (!str)
	{
		return NULL;
	}
	len = strlen(str);

	for (idx = 0; idx <= len; idx++)
	{
		if ((str[idx] == dilim) || (idx == len))
		{
			if (counter == token_number)
			{
				substring = xmlrpc_myStrSubString(str, start_pos, len);
				counter++;
			}
			else
			{
				start_pos = idx + 1;
				counter++;
			}
		}
	}
	return substring;
}

/*************************************************************************/

/**
 * Get the string between point A and point B
 * @param str String to search in
 * @param start Point A
 * @param end Point B
 * @return the string in between
 */
static char *xmlrpc_myStrSubString(const char *src, int start, int end)
{
	char *substring = NULL;
	int len, idx;
	if (!src)
	{
		return NULL;
	}
	len = strlen(src);
	if (((start >= 0) && (end <= len)) && (end > start))
	{
		substring = (char *)malloc(sizeof(char) * ((end - start) + 1));
		for (idx = 0; idx <= end - start; idx++)
		{
			substring[idx] = src[start + idx];
		}
		substring[end - start] = '\0';
	}
	return substring;
}

/*************************************************************************/

char *xmlrpc_normalizeBuffer(char *buf)
{
	char *newbuf;
	int i, len, j = 0;

	len = strlen(buf);
	newbuf = (char *)malloc(sizeof(char) * len + 1);

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
			  newbuf[j] = buf[i];
			  j++;
		}
	}

	/* Terminate the string */
	newbuf[j] = 0;

	return (newbuf);
}

/*************************************************************************/

char *xmlrpc_stristr(char *s1, char *s2)
{
	register char *s = s1, *d = s2;

	while (*s1)
	{
		if (xmlrpc_tolower(*s1) == xmlrpc_tolower(*d))
		{
			s1++;
			d++;
			if (*d == 0)
				return s;
		}
		else
		{
			s = ++s1;
			d = s2;
		}
	}
	return NULL;
}

/*************************************************************************/

static int xmlrpc_tolower(char c)
{
	if (isupper(c))
	{
		return (unsigned char)c + ('a' - 'A');
	}
	else
	{
		return (unsigned char)c;
	}
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
			xmlrpc.encode = xmlrpc_strdup(value);
		}
	}
	if (type == XMLRPC_INTTAG)
	{
		if (!stricmp(value, XMLRPC_I4))
		{
			xmlrpc.inttagstart = xmlrpc_strdup("<i4>");
			xmlrpc.inttagend = xmlrpc_strdup("</i4>");
		}
		if (!stricmp(value, XMLRPC_INT))
		{
			xmlrpc.inttagstart = xmlrpc_strdup("<int>");
			xmlrpc.inttagend = xmlrpc_strdup("</int>");
		}
	}
	return 1;
}

/*************************************************************************/

char *xmlrpc_char_encode(char *outbuffer, char *s1)
{
	int i;
	unsigned char c;
	char buf2[15];
	char buf3[XMLRPC_BUFSIZE];
	*buf2 = '\0';
	*buf3 = '\0';
	*outbuffer = '\0';

	if ((!(s1) || (*(s1) == '\0')))
	{
		return (char *)"";
	}

	for (i = 0; i <= (int)strlen(s1) - 1; i++)
	{
		c = s1[i];
		if (c > 127)
		{
			snprintf(buf2, 15, "&#%ld;", (long int)c);
			if (outbuffer)
			{
				snprintf(buf3, XMLRPC_BUFSIZE, "%s%s", outbuffer, buf2);
			}
			else
			{
				snprintf(buf3, XMLRPC_BUFSIZE, "%s", buf2);
			}
			snprintf(outbuffer, XMLRPC_BUFSIZE, "%s", buf3);
		}
		else if (c == '&')
		{
			if (outbuffer)
			{
				snprintf(buf3, XMLRPC_BUFSIZE, "%s%s", outbuffer, "&amp;");
			}
			else
			{
				snprintf(buf3, XMLRPC_BUFSIZE, "%s", "&amp;");
			}
			snprintf(outbuffer, XMLRPC_BUFSIZE, "%s", buf3);
		}
		else if (c == '<')
		{
			if (outbuffer)
			{
				snprintf(buf3, XMLRPC_BUFSIZE, "%s%s", outbuffer, "&lt;");
			}
			else
			{
				snprintf(buf3, XMLRPC_BUFSIZE, "%s", "&lt;");
			}
			snprintf(outbuffer, XMLRPC_BUFSIZE, "%s", buf3);
		}
		else if (c == '>')
		{
			if (outbuffer)
			{
				snprintf(buf3, XMLRPC_BUFSIZE, "%s%s", outbuffer, "&gt;");
			}
			else
			{
				snprintf(buf3, XMLRPC_BUFSIZE, "%s", "&gt;");
			}
			snprintf(outbuffer, XMLRPC_BUFSIZE, "%s", buf3);
		}
		else if (c == '"')
		{
			if (outbuffer)
			{
				snprintf(buf3, XMLRPC_BUFSIZE, "%s%s", outbuffer, "&quot;");
			}
			else
			{
				snprintf(buf3, XMLRPC_BUFSIZE, "%s", "&quot;");
			}
			snprintf(outbuffer, XMLRPC_BUFSIZE, "%s", buf3);
		}
		else
		{
			if (outbuffer)
			{
				snprintf(buf3, XMLRPC_BUFSIZE, "%s%c", outbuffer, c);
			}
			else
			{
				snprintf(buf3, XMLRPC_BUFSIZE, "%c", c);
			}
			snprintf(outbuffer, XMLRPC_BUFSIZE, "%s", buf3);
		}
	}

	return outbuffer;
}

char *xmlrpc_decode_string(char *buf)
{
	int count;
	int i;
	char *token, *temp;
	char *temptoken;
	char buf2[12];
	char buf3[12];

	xmlrcp_strnrepl(buf, XMLRPC_BUFSIZE, "&gt;", ">");
	xmlrcp_strnrepl(buf, XMLRPC_BUFSIZE, "&lt;", "<");
	xmlrcp_strnrepl(buf, XMLRPC_BUFSIZE, "&quot;", "\"");
	xmlrcp_strnrepl(buf, XMLRPC_BUFSIZE, "&amp;", "&");

	temp = xmlrpc_strdup(buf);
	count = xmlrpc_myNumToken(temp, '&');
	for (i = 1; i <= count; i++)
	{
		token = xmlrpc_GetToken(temp, '&', i);
		if (token && *token == '#')
		{
			temptoken = strtok((token + 1), ";");
			snprintf(buf2, 12, "%c", atoi(temptoken));
			snprintf(buf3, 12, "&%s;", token);
			xmlrcp_strnrepl(buf, XMLRPC_BUFSIZE, buf3, buf2);
		}
	}
	free(temp);
	return buf;
}

char *xmlrcp_strnrepl(char *s, int32_t size, const char *old, const char *new)
{
	char *ptr = s;
	int32_t left = strlen(s);
	int32_t avail = size - (left + 1);
	int32_t oldlen = strlen(old);
	int32_t newlen = strlen(new);
	int32_t diff = newlen - oldlen;

	while (left >= oldlen)
	{
		if (strncmp(ptr, old, oldlen) != 0)
		{
			left--;
			ptr++;
			continue;
		}
		if (diff > avail)
			break;
		if (diff != 0)
			memmove(ptr + oldlen + diff, ptr + oldlen, left + 1);
		strncpy(ptr, new, newlen);
		ptr += newlen;
		left -= oldlen;
	}
	return s;
}

int xmlrpc_myNumToken(const char *str, const char dilim)
{
	int len, idx, counter = 0, start_pos = 0;
	if (!str)
	{
		return 0;
	}

	len = strlen(str);
	for (idx = 0; idx <= len; idx++)
	{
		if ((str[idx] == dilim) || (idx == len))
		{
			start_pos = idx + 1;
			counter++;
		}
	}
	return counter;
}
