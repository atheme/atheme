/* JSONRPC Library
 *
 * Copyright (c) 2014 Atheme Development Group
 * Please read COPYING and README for further details.
 *
 */

#include <mowgli.h>
#include "atheme.h"
#include "jsonrpclib.h"

void jsonrpc_process(char *buffer, void *userdata)
{
	if (!buffer)
	{
		return;
	}

	mowgli_json_t *parsed = mowgli_json_parse_string(buffer);

	if (parsed == NULL) {
		return;
	}

	mowgli_json_tag_t tag = MOWGLI_JSON_TAG(parsed);

	//JSON RPC works with JSON objects only, anything else can't be correct.

	if (tag != MOWGLI_JSON_TAG_OBJECT)
	{
		return;
	}

	mowgli_patricia_t *obj = MOWGLI_JSON_OBJECT(parsed);

	if (parsed == NULL)
	{
		return;
	}

	mowgli_json_t *method = mowgli_patricia_retrieve(obj, "method");
	mowgli_json_t *params = mowgli_patricia_retrieve(obj, "params");
	mowgli_json_t *id = mowgli_patricia_retrieve(obj, "id");

	char *method_str, *id_str;
	mowgli_list_t *params_list;

	if (id == NULL || params == NULL || method == NULL)
	{
		return;
	}

	if (MOWGLI_JSON_TAG(method) != MOWGLI_JSON_TAG_STRING ||
			MOWGLI_JSON_TAG(id) != MOWGLI_JSON_TAG_STRING ||
			MOWGLI_JSON_TAG(params) != MOWGLI_JSON_TAG_ARRAY)
	{
		return;
	}

	method_str = MOWGLI_JSON_STRING_STR(method);
	id_str = MOWGLI_JSON_STRING_STR(id);
	params_list = MOWGLI_JSON_ARRAY(params);

	mowgli_json_t *param;
	mowgli_node_t *n;

	jsonrpc_method_t call_method = get_json_method(method_str);

	MOWGLI_LIST_FOREACH(n, params_list->head)
	{
		param = n->data;

		if (MOWGLI_JSON_TAG(param) != MOWGLI_JSON_TAG_STRING) {
			continue;
		}

	}

	mowgli_list_t *params_str = mowgli_list_create();

	MOWGLI_LIST_FOREACH(n, params_list->head)
	{
		param = n->data;

		char *param_str = MOWGLI_JSON_STRING_STR(param);
		mowgli_node_add(param_str, mowgli_node_create(), params_str);
	}

	if (call_method != NULL) {
		call_method(userdata, params_str, id_str);
	} else {
		jsonrpc_failure_string(userdata, fault_badparams, "Invalid command", id_str);
		return;
	}

}

void jsonrpc_success_string(void *conn, const char *result, const char *id)
{
	mowgli_json_t *obj = mowgli_json_create_object();

	mowgli_patricia_t *patricia = MOWGLI_JSON_OBJECT(obj);

	mowgli_json_t *resultobj = mowgli_json_create_string(result);
	mowgli_json_t *idobj = mowgli_json_create_string(id);

	mowgli_patricia_add(patricia, "result", resultobj);
	mowgli_patricia_add(patricia, "id", idobj);
	mowgli_patricia_add(patricia, "error", mowgli_json_null);

	mowgli_string_t *str = mowgli_string_create();

	mowgli_json_serialize_to_string(obj, str, 0);

	jsonrpc_send_data(conn, str->str);
}

void jsonrpc_failure_string(void *conn, int code, const char *error, const char *id)
{
	mowgli_json_t *obj = mowgli_json_create_object();
	mowgli_json_t *errorobj = mowgli_json_create_object();

	mowgli_patricia_t *patricia = MOWGLI_JSON_OBJECT(errorobj);

	mowgli_patricia_add(patricia, "code", mowgli_json_create_integer(code));
	mowgli_patricia_add(patricia, "message", mowgli_json_create_string(error));

	patricia = MOWGLI_JSON_OBJECT(obj);

	mowgli_json_t *idobj = mowgli_json_create_string(id);

	mowgli_patricia_add(patricia, "result", mowgli_json_null);
	mowgli_patricia_add(patricia, "id", idobj);
	mowgli_patricia_add(patricia, "error", errorobj);

	mowgli_string_t *str = mowgli_string_create();

	mowgli_json_serialize_to_string(obj, str, 0);

	jsonrpc_send_data(conn, str->str);
}

char *jsonrpc_normalizeBuffer(const char *buf)
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
				if (isdigit((unsigned char)buf[i + 1]))
				{
					i++;

					/* not the best way to remove colors
					 * which are two digit but no worse then
					 * how the Unreal does with +S - TSL
					 */
					if (isdigit((unsigned char)buf[i + 1]))
					{
						i++;
					}

					/* Check for background color code
					 * and remove it as well
					 */
					if (buf[i + 1] == ',')
					{
						i++;

						if (isdigit((unsigned char)buf[i + 1]))
						{
							i++;
						}
						/* not the best way to remove colors
						 * which are two digit but no worse then
						 * how the Unreal does with +S - TSL
						 */
						if (isdigit((unsigned char)buf[i + 1]))
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

