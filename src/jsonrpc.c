/* JSON RPC Library
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

#include "jsonrpc.h"

int jsonrpc_error_code;

JSONRPCCmdHash *JSONRPCCMD[MAX_CMD_HASH];

JSONRPCSet jsonrpc;

static int jsonrpc_tolower(char c);
static char *jsonrpc_GetToken(const char *str, const char dilim, int token_number);
static char *jsonrpc_GetTokenRemainder(const char *str, const char dilim, int token_number);
static char *jsonrpc_myStrSubString(const char *src, int start, int end);
static char *jsonrpc_strdup(const char *src);
static char *jsonrpc_parse(char *buffer);
static char *jsonrpc_method(char *buffer);
static char *jsonrpc_normalizeBuffer(char *buf);
static char *jsonrpc_stristr(char *s1, char *s2);
static int jsonrpc_split_buf(char *buffer, char ***argv);

JSONRPCCmd *createJSONCommand(const char *name, JSONRPCMethodFunc func);
JSONRPCCmd *findJSONRPCCommand(JSONRPCCmdHash * hookEvtTable[], const char *name);
int addJSONCommand(JSONRPCCmdHash * hookEvtTable[], JSONRPCCmd * json);
int destroyJSONRPCCommand(JSONRPCCmd * json);
int delJSONRPCCommand(JSONRPCCmdHash * msgEvtTable[], JSONRPCCmd * json, char *mod_name);
char *jsonrpc_write_header(int length);
int addCoreJSONRPCCmd(JSONRPCCmdHash * hookEvtTable[], JSONRPCCmd * json);
JSONRPCCmd *first_jsonrpccmd(void);
JSONRPCCmd *next_jsonrpccmd(void);
JSONRPCCmdHash *first_jsonrpchash(void);
JSONRPCCmdHash *next_jsonrpchash(void);
int destroyjsonrpchash(JSONRPCCmdHash * mh);
char *jsonrcp_strnrepl(char *s, int size, const char *old, const char *new);
int jsonrpc_myNumToken(const char *str, const char dilim);

/*************************************************************************/

int jsonrpc_getlast_error(void)
{
	if (!jsonrpc_error_code)
	{
		return 0;
	}
	else
	{
		return jsonrpc_error_code;
	}
}

/*************************************************************************/

void jsonrpc_process(char *buffer, void *userdata)
{
	int retVal = 0;
	JSONRPCCmd *current = NULL;
	JSONRPCCmd *json;
	char *tmp;
	int ac = 0;
	char **av;
	char *name = NULL;

	jsonrpc_error_code = 0;

	if (!buffer)
	{
		jsonrpc_error_code = -1;
		return;
	}

	tmp = jsonrpc_parse(buffer);
	if (tmp)
	{
		name = jsonrpc_method(tmp);
		if (name)
		{
			json = findJSONRPCCommand(JSONRPCCMD, name);
			if (json)
			{
				ac = jsonrpc_split_buf(tmp, &av);
				if (json->func)
				{
					retVal = json->func(userdata, ac, av);
					if (retVal == JSONRPC_CONT)
					{
						current = json->next;
						while (current && current->func && retVal == JSONRPC_CONT)
						{
							retVal = current->func(userdata, ac, av);
							current = current->next;
						}
					}
					else
					{	/* we assume that JSONRPC_STOP means the handler has given no output */
						jsonrpc_error_code = -7;
						jsonrpc_generic_error(jsonrpc_error_code, "JSONRPC error: First eligible function returned JSONRPC_STOP");
					}
				}
				else
				{
					jsonrpc_error_code = -6;
					jsonrpc_generic_error(jsonrpc_error_code, "JSONRPC error: Method has no registered function");
				}
			}
			else
			{
				jsonrpc_error_code = -4;
				jsonrpc_generic_error(jsonrpc_error_code, "JSONRPC error: Unknown routine called");
			}
		}
		else
		{
			jsonrpc_error_code = -3;
			jsonrpc_generic_error(jsonrpc_error_code, "JSONRPC error: Missing methodRequest or methodName.");
		}
	}
	else
	{
		jsonrpc_error_code = -2;
		jsonrpc_generic_error(jsonrpc_error_code, "JSONRPC error: Invalid document end at line 1");
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

void jsonrpc_set_buffer(char *(*func) (char *buffer, int len))
{
	jsonrpc.setbuffer = NULL;
	if (func)
	{
		jsonrpc.setbuffer = func;
	}
	else
	{
		jsonrpc_error_code = -8;
	}
}

/*************************************************************************/

int jsonrpc_register_method(const char *name, JSONRPCMethodFunc func)
{
	JSONRPCCmd *json;
	json = createJSONCommand(name, func);
	return addJSONCommand(JSONRPCCMD, json);
}

/*************************************************************************/

JSONRPCCmd *createJSONCommand(const char *name, JSONRPCMethodFunc func)
{
	JSONRPCCmd *json = NULL;
	if (!func)
	{
		return NULL;
	}
	if ((json = malloc(sizeof(JSONRPCCmd))) == NULL)
	{
		return NULL;
	}
	json->name = jsonrpc_strdup(name);
	json->func = func;
	json->next = NULL;
	return json;
}

/*************************************************************************/

JSONRPCCmd *findJSONRPCCommand(JSONRPCCmdHash * hookEvtTable[], const char *name)
{
	int idx;
	JSONRPCCmdHash *current = NULL;
	if (!hookEvtTable || !name)
	{
		return NULL;
	}
	idx = CMD_HASH(name);

	for (current = hookEvtTable[idx]; current; current = current->next)
	{
		if (stricmp(name, current->name) == 0)
		{
			return current->json;
		}
	}
	return NULL;
}

/*************************************************************************/

int addJSONCommand(JSONRPCCmdHash * hookEvtTable[], JSONRPCCmd * json)
{
	/* We can assume both param's have been checked by this point.. */
	int idx = 0;
	JSONRPCCmdHash *current = NULL;
	JSONRPCCmdHash *newHash = NULL;
	JSONRPCCmdHash *lastHash = NULL;

	if (!hookEvtTable || !json)
	{
		return JSONRPC_ERR_PARAMS;
	}
	idx = CMD_HASH(json->name);

	for (current = hookEvtTable[idx]; current; current = current->next)
	{
		if (stricmp(json->name, current->name) == 0)
		{
			json->next = current->json;
			current->json = json;
			return JSONRPC_ERR_OK;
		}
		lastHash = current;
	}

	if ((newHash = malloc(sizeof(JSONRPCCmdHash))) == NULL)
	{
		return JSONRPC_ERR_OK;
	}
	newHash->next = NULL;
	newHash->name = jsonrpc_strdup(json->name);
	newHash->json = json;

	if (lastHash == NULL)
		hookEvtTable[idx] = newHash;
	else
		lastHash->next = newHash;
	return JSONRPC_ERR_OK;
}

/*************************************************************************/

int jsonrpc_unregister_method(const char *method)
{
	JSONRPCCmd *json;

	json = findJSONRPCCommand(JSONRPCCMD, method);
	return destroyJSONRPCCommand(json);
}

/*************************************************************************/

/**
 * Destory a message, freeing its memory.
 * @param m the message to be destroyed
 * @return JSONRPC_ERR_SUCCESS on success
 **/
int destroyJSONRPCCommand(JSONRPCCmd * json)
{
	if (!json)
	{
		return JSONRPC_ERR_PARAMS;
	}
	if (json->name)
	{
		free(json->name);
	}
	json->func = NULL;
	if (json->mod_name)
	{
		free(json->mod_name);
	}
	json->next = NULL;
	return JSONRPC_ERR_OK;
}

/*************************************************************************/

int delJSONRPCCommand(JSONRPCCmdHash * msgEvtTable[], JSONRPCCmd * json, char *mod_name)
{
	int idx = 0;
	JSONRPCCmdHash *current = NULL;
	JSONRPCCmdHash *lastHash = NULL;
	JSONRPCCmd *tail = NULL, *last = NULL;

	if (!json || !msgEvtTable)
	{
		return JSONRPC_ERR_PARAMS;
	}
	idx = CMD_HASH(json->name);

	for (current = msgEvtTable[idx]; current; current = current->next)
	{
		if (stricmp(json->name, current->name) == 0)
		{
			if (!lastHash)
			{
				tail = current->json;
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
								current->json = tail->next;
							}
							return JSONRPC_ERR_OK;
						}
						last = tail;
						tail = tail->next;
					}
				}
				else
				{
					msgEvtTable[idx] = current->next;
					free(current->name);
					return JSONRPC_ERR_OK;
				}
			}
			else
			{
				tail = current->json;
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
								current->json = tail->next;
							}
							return JSONRPC_ERR_OK;
						}
						last = tail;
						tail = tail->next;
					}
				}
				else
				{
					lastHash->next = current->next;
					free(current->name);
					return JSONRPC_ERR_OK;
				}
			}
		}
		lastHash = current;
	}
	return JSONRPC_ERR_NOEXIST;
}

/*************************************************************************/

int addCoreJSONRPCCmd(JSONRPCCmdHash * hookEvtTable[], JSONRPCCmd * json)
{
	if (!hookEvtTable || !json)
	{
		return JSONRPC_ERR_PARAMS;
	}
	json->core = 1;
	return addJSONCommand(hookEvtTable, json);
}

/*************************************************************************/

char *jsonrpc_write_header(int length)
{
	char buf[JSONRPC_BUFSIZE];
	time_t ts;
	char timebuf[64];
	struct tm tm;

	*buf = '\0';

	ts = time(NULL);
	tm = *localtime(&ts);
	strftime(timebuf, JSONRPC_BUFSIZE - 1, "%Y-%m-%d %H:%M:%S", &tm);

	snprintf(buf, JSONRPC_BUFSIZE, "HTTP/1.1 200 OK\r\nConnection: close\r\n" "Content-Length: %d\r\n" "Content-Type: text/json\r\n" "Date: %s\r\n" "Server: Atheme/%s\r\n\r\n", length, timebuf, version);
	return jsonrpc_strdup(buf);
}

/*************************************************************************/

/**
 * Parse Data down to just the <json> without any \r\n or \t
 * @param buffer Incoming data buffer
 * @return cleaned up buffer
 */
char *jsonrpc_parse(char *buffer)
{
	char *tmp = NULL;

	/*
	   Okay since the buffer could contain 
	   HTTP header information, lets break
	   off at the point that the <?json?> starts
	 */
	tmp = jsonrpc_stristr(buffer, (char *)"<?json");

	/* check if its json doc */
	if (tmp)
	{
		/* get all the odd characters out of the data */
		return jsonrpc_normalizeBuffer(tmp);
	}
	return NULL;
}

/*************************************************************************/

static int jsonrpc_split_buf(char *buffer, char ***argv)
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
			temp = jsonrpc_GetToken(data, '<', 2);
			nexttag = jsonrpc_GetToken(temp, '>', 0);
			/* strings */
			if (!stricmp("string", nexttag))
			{
				tagtype = 1;
			}
			else
			{
				tagtype = 0;
			}
			str = jsonrpc_GetToken(data, '>', 2);
			str2 = jsonrpc_GetTokenRemainder(data, '>', 2);
			if (str)
			{
				final = jsonrpc_GetToken(str, '<', 0);
				if (final)
				{
					if (tagtype == 1)
					{
						(*argv)[ac++] = jsonrpc_decode_string(final);
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

char *jsonrpc_method(char *buffer)
{
	char *data;
	char *tmp, *tmp2;

	data = jsonrpc_stristr(buffer, (char *)"<methodname>");
	if (data)
	{
		tmp = jsonrpc_GetToken(data, '>', 1);
		tmp2 = jsonrpc_GetToken(tmp, '<', 0);
		free(tmp);
		return tmp2;
	}
	return NULL;
}

/*************************************************************************/

void jsonrpc_generic_error(int code, const char *string)
{
	char buf[JSONRPC_BUFSIZE], buf2[JSONRPC_BUFSIZE];
	int len;

	snprintf(buf, JSONRPC_BUFSIZE,
		 "<?json version=\"1.0\"?>\r\n<methodResponse>\r\n <fault>\r\n  <value>\r\n   <struct>\r\n    <member>\r\n     <name>faultCode</name>\r\n     <value><int>%d</int></value>\r\n    </member>\r\n    <member>\r\n     <name>faultString</name>\r\n     <value><string>%s</string></value>\r\n    </member>\r\n   </struct>\r\n  </value>\r\n </fault>\r\n</methodResponse>",
		 code, string);
	len = strlen(buf);

	if (jsonrpc.httpheader)
	{
		char *header = jsonrpc_write_header(len);
		strlcpy(buf2, header, JSONRPC_BUFSIZE);
		strlcat(buf2, buf, JSONRPC_BUFSIZE);
		len += strlen(header);
		free(header);
		jsonrpc.setbuffer(buf2, len);
	}
	else
		jsonrpc.setbuffer(buf, len);
}

/*************************************************************************/

int jsonrpc_about(void *userdata, int ac, char **av)
{
	char buf[JSONRPC_BUFSIZE];
	char buf2[JSONRPC_BUFSIZE];
	char buf3[JSONRPC_BUFSIZE];
	char buf4[JSONRPC_BUFSIZE];
	char *arraydata;

	(void)userdata;
	jsonrpc_integer(buf3, ac);
	jsonrpc_string(buf4, av[0]);
	jsonrpc_string(buf, (char *)JSONLIB_VERSION);
	jsonrpc_string(buf2, (char *)JSONLIB_AUTHOR);
	arraydata = jsonrpc_array(4, buf, buf2, buf3, buf4);

	jsonrpc_send(1, arraydata);
	free(arraydata);
	return JSONRPC_CONT;
}

/*************************************************************************/

void jsonrpc_send(int argc, ...)
{
	va_list va;
	char *a;
	int idx = 0;
	char *s = NULL;
	int len;
	char buf[JSONRPC_BUFSIZE];
	char buf2[JSONRPC_BUFSIZE];
	char *header;

	*buf = '\0';
	*buf2 = '\0';

	va_start(va, argc);
	for (idx = 0; idx < argc; idx++)
	{
		a = va_arg(va, char *);
		if (!s)
		{
			snprintf(buf, JSONRPC_BUFSIZE, " <param>\r\n  <value>\r\n   %s\r\n  </value>\r\n </param>", a);
			s = jsonrpc_strdup(buf);
		}
		else
		{
			snprintf(buf, JSONRPC_BUFSIZE, "%s\r\n <param>\r\n  <value>\r\n   %s\r\n  </value>\r\n </param>", s, a);
			free(s);
			s = jsonrpc_strdup(buf);
		}
	}
	va_end(va);

	if (jsonrpc.encode)
	{
		snprintf(buf, JSONRPC_BUFSIZE, "<?json version=\"1.0\" encoding=\"%s\" ?>\r\n<methodResponse>\r\n<params>\r\n%s\r\n</params>\r\n</methodResponse>", jsonrpc.encode, s);
	}
	else
	{
		snprintf(buf, JSONRPC_BUFSIZE, "<?json version=\"1.0\"?>\r\n<methodResponse>\r\n<params>\r\n%s\r\n</params>\r\n</methodResponse>", s);
	}
	len = strlen(buf);

	if (jsonrpc.httpheader)
	{
		header = jsonrpc_write_header(len);
		snprintf(buf2, JSONRPC_BUFSIZE, "%s%s", header, buf);
		free(header);
		len = strlen(buf2);
		jsonrpc.setbuffer(buf2, len);
		jsonrpc.httpheader = 1;
	}
	else
	{
		jsonrpc.setbuffer(buf, len);
	}
	if (jsonrpc.encode)
	{
		free(jsonrpc.encode);
		jsonrpc.encode = NULL;
	}
	free(s);
}

/*************************************************************************/

char *jsonrpc_time2date(char *buf, time_t t)
{
	char timebuf[JSONRPC_BUFSIZE];
	struct tm *tm;

	*buf = '\0';
	tm = localtime(&t);
	/* <dateTime.iso8601>20011003T08:53:38</dateTime.iso8601> */
	strftime(timebuf, JSONRPC_BUFSIZE - 1, "%Y%m%dT%I:%M:%S", tm);
	snprintf(buf, JSONRPC_BUFSIZE, "<dateTime.iso8601>%s</dateTime.iso8601>", timebuf);
	return buf;
}

/*************************************************************************/

char *jsonrpc_integer(char *buf, int value)
{
	*buf = '\0';

	if (!jsonrpc.inttagstart || !jsonrpc.inttagend)
	{
		snprintf(buf, JSONRPC_BUFSIZE, "<i4>%d</i4>", value);
	}
	else
	{
		snprintf(buf, JSONRPC_BUFSIZE, "%s%d%s", jsonrpc.inttagstart, value, jsonrpc.inttagend);
		free(jsonrpc.inttagstart);
		if (jsonrpc.inttagend)
		{
			free(jsonrpc.inttagend);
			jsonrpc.inttagend = NULL;
		}
		jsonrpc.inttagstart = NULL;
	}
	return buf;
}

/*************************************************************************/

char *jsonrpc_string(char *buf, const char *value)
{
	char encoded[JSONRPC_BUFSIZE];
	*buf = '\0';

	jsonrpc_char_encode(encoded, value);
	snprintf(buf, JSONRPC_BUFSIZE, "<string>%s</string>", encoded);
	return buf;
}

/*************************************************************************/

char *jsonrpc_boolean(char *buf, int value)
{
	*buf = '\0';
	snprintf(buf, JSONRPC_BUFSIZE, "<boolean>%d</boolean>", (value ? 1 : 0));
	return buf;
}

/*************************************************************************/

char *jsonrpc_double(char *buf, double value)
{
	*buf = '\0';
	snprintf(buf, JSONRPC_BUFSIZE, "<double>%g</double>", value);
	return buf;
}

/*************************************************************************/

char *jsonrpc_array(int argc, ...)
{
	va_list va;
	char *a;
	int idx = 0;
	char *s = NULL;
	int len;
	char buf[JSONRPC_BUFSIZE];

	va_start(va, argc);
	for (idx = 0; idx < argc; idx++)
	{
		a = va_arg(va, char *);
		if (!s)
		{
			snprintf(buf, JSONRPC_BUFSIZE, "   <value>%s</value>", a);
			s = jsonrpc_strdup(buf);
		}
		else
		{
			snprintf(buf, JSONRPC_BUFSIZE, "%s\r\n     <value>%s</value>", s, a);
			free(s);
			s = jsonrpc_strdup(buf);
		}
	}
	va_end(va);

	snprintf(buf, JSONRPC_BUFSIZE, "<array>\r\n    <data>\r\n  %s\r\n    </data>\r\n   </array>", s);
	len = strlen(buf);
	free(s);
	return jsonrpc_strdup(buf);
}

/*************************************************************************/

static JSONRPCCmdHash *current;
static int next_index;

JSONRPCCmd *first_jsonrpccmd(void)
{
	next_index = 0;

	while (next_index < 1024 && current == NULL)
		current = JSONRPCCMD[next_index++];
	if (current)
	{
		return current->json;
	}
	else
	{
		return NULL;
	}
}

/*************************************************************************/

JSONRPCCmd *next_jsonrpccmd(void)
{
	if (current)
		current = current->next;
	if (!current && next_index < 1024)
	{
		while (next_index < 1024 && current == NULL)
			current = JSONRPCCMD[next_index++];
	}
	if (current)
	{
		return current->json;
	}
	else
	{
		return NULL;
	}
}

/*************************************************************************/

JSONRPCCmdHash *first_jsonrpchash(void)
{
	next_index = 0;

	while (next_index < 1024 && current == NULL)
		current = JSONRPCCMD[next_index++];
	return current;
}

/*************************************************************************/

JSONRPCCmdHash *next_jsonrpchash(void)
{
	if (current)
		current = current->next;
	if (!current && next_index < 1024)
	{
		while (next_index < 1024 && current == NULL)
			current = JSONRPCCMD[next_index++];
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

int destroyjsonrpchash(JSONRPCCmdHash * mh)
{
	if (!mh)
	{
		return JSONRPC_ERR_PARAMS;
	}
	if (mh->name)
	{
		free(mh->name);
	}
	mh->json = NULL;
	mh->next = NULL;
	free(mh);
	return JSONRPC_ERR_OK;
}

/*************************************************************************/

static char *jsonrpc_strdup(const char *src)
{
	char *ret = NULL;

	if (src)
	{
		if ((ret = (char *)malloc(strlen(src) + 1)) != NULL)
			strcpy(ret, src);
		else
			raise(SIGUSR1);
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
static char *jsonrpc_GetToken(const char *str, const char dilim, int token_number)
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
				substring = jsonrpc_myStrSubString(str, start_pos, idx);
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
static char *jsonrpc_GetTokenRemainder(const char *str, const char dilim, int token_number)
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
				substring = jsonrpc_myStrSubString(str, start_pos, len);
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
static char *jsonrpc_myStrSubString(const char *src, int start, int end)
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

char *jsonrpc_normalizeBuffer(char *buf)
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

char *jsonrpc_stristr(char *s1, char *s2)
{
	register char *s = s1, *d = s2;

	while (*s1)
	{
		if (jsonrpc_tolower(*s1) == jsonrpc_tolower(*d))
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

static int jsonrpc_tolower(char c)
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

int jsonrpc_set_options(int type, const char *value)
{
	if (type == JSONRPC_HTTP_HEADER)
	{
		if (!stricmp(value, JSONRPC_ON))
		{
			jsonrpc.httpheader = 1;
		}
		if (!stricmp(value, JSONRPC_OFF))
		{
			jsonrpc.httpheader = 0;
		}
	}
	if (type == JSONRPC_ENCODE)
	{
		if (value)
		{
			jsonrpc.encode = jsonrpc_strdup(value);
		}
	}
	if (type == JSONRPC_INTTAG)
	{
		if (!stricmp(value, JSONRPC_I4))
		{
			jsonrpc.inttagstart = jsonrpc_strdup("<i4>");
			jsonrpc.inttagend = jsonrpc_strdup("</i4>");
		}
		if (!stricmp(value, JSONRPC_INT))
		{
			jsonrpc.inttagstart = jsonrpc_strdup("<int>");
			jsonrpc.inttagend = jsonrpc_strdup("</int>");
		}
	}
	return 1;
}

/*************************************************************************/

char *jsonrpc_char_encode(char *outbuffer, const char *s1)
{
	long unsigned int i;
	unsigned char c;
	char buf2[15];
	char buf3[JSONRPC_BUFSIZE];
	*buf2 = '\0';
	*buf3 = '\0';
	*outbuffer = '\0';

	if ((!(s1) || (*(s1) == '\0')))
	{
		return (char *)"";
	}

	for (i = 0; i <= strlen(s1) - 1; i++)
	{
		c = s1[i];
		if (c > 127)
		{
			snprintf(buf2, 15, "&#%c;", c);
			if (outbuffer)
			{
				snprintf(buf3, JSONRPC_BUFSIZE, "%s%s", outbuffer, buf2);
			}
			else
			{
				snprintf(buf3, JSONRPC_BUFSIZE, "%s", buf2);
			}
			snprintf(outbuffer, JSONRPC_BUFSIZE, "%s", buf3);
		}
		else if (c == '&')
		{
			if (outbuffer)
			{
				snprintf(buf3, JSONRPC_BUFSIZE, "%s%s", outbuffer, "&amp;");
			}
			else
			{
				snprintf(buf3, JSONRPC_BUFSIZE, "%s", "&amp;");
			}
			snprintf(outbuffer, JSONRPC_BUFSIZE, "%s", buf3);
		}
		else if (c == '<')
		{
			if (outbuffer)
			{
				snprintf(buf3, JSONRPC_BUFSIZE, "%s%s", outbuffer, "&lt;");
			}
			else
			{
				snprintf(buf3, JSONRPC_BUFSIZE, "%s", "&lt;");
			}
			snprintf(outbuffer, JSONRPC_BUFSIZE, "%s", buf3);
		}
		else if (c == '>')
		{
			if (outbuffer)
			{
				snprintf(buf3, JSONRPC_BUFSIZE, "%s%s", outbuffer, "&gt;");
			}
			else
			{
				snprintf(buf3, JSONRPC_BUFSIZE, "%s", "&gt;");
			}
			snprintf(outbuffer, JSONRPC_BUFSIZE, "%s", buf3);
		}
		else if (c == '"')
		{
			if (outbuffer)
			{
				snprintf(buf3, JSONRPC_BUFSIZE, "%s%s", outbuffer, "&quot;");
			}
			else
			{
				snprintf(buf3, JSONRPC_BUFSIZE, "%s", "&quot;");
			}
			snprintf(outbuffer, JSONRPC_BUFSIZE, "%s", buf3);
		}
		else
		{
			if (outbuffer)
			{
				snprintf(buf3, JSONRPC_BUFSIZE, "%s%c", outbuffer, c);
			}
			else
			{
				snprintf(buf3, JSONRPC_BUFSIZE, "%c", c);
			}
			snprintf(outbuffer, JSONRPC_BUFSIZE, "%s", buf3);
		}
	}

	return outbuffer;
}

char *jsonrpc_decode_string(char *buf)
{
	int count;
	int i;
	char *token, *temp;
	char *temptoken;
	char buf2[12];
	char buf3[12];

	jsonrcp_strnrepl(buf, JSONRPC_BUFSIZE, "&gt;", ">");
	jsonrcp_strnrepl(buf, JSONRPC_BUFSIZE, "&lt;", "<");
	jsonrcp_strnrepl(buf, JSONRPC_BUFSIZE, "&quot;", "\"");
	jsonrcp_strnrepl(buf, JSONRPC_BUFSIZE, "&amp;", "&");

	temp = jsonrpc_strdup(buf);
	count = jsonrpc_myNumToken(temp, '&');
	for (i = 1; i <= count; i++)
	{
		token = jsonrpc_GetToken(temp, '&', i);
		if (token && *token == '#')
		{
			temptoken = strtok((token + 1), ";");
			snprintf(buf2, 12, "%c", atoi(temptoken));
			snprintf(buf3, 12, "&%s;", token);
			jsonrcp_strnrepl(buf, JSONRPC_BUFSIZE, buf3, buf2);
		}
	}
	free(temp);
	return buf;
}

char *jsonrcp_strnrepl(char *s, int size, const char *old, const char *new)
{
	char *ptr = s;
	int left = strlen(s);
	int avail = size - (left + 1);
	int oldlen = strlen(old);
	int newlen = strlen(new);
	int diff = newlen - oldlen;

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

int jsonrpc_myNumToken(const char *str, const char dilim)
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

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
