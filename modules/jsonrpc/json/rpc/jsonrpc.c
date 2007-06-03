#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "json/json.h"
#include "json/arraylist.h"
#include "json/rpc/hashmap.h"
#include "json/rpc/circular_buffer.h"
#include "json/rpc/jsonrpc.h"

/*****************************************************************************/

typedef struct jsonrpc_event
{
	long id;
	char *type;
	time_t tm;
	struct json_object *data;
} jsonrpc_event;

/*****************************************************************************/

hashmap methodMap;
circular_buffer eventBus;
jsonrpc_event *spareEvent;
long eventIds;

/*****************************************************************************/

struct json_object *jsonrpc_clone_object(struct json_object *object)
{
	return json_tokener_parse(json_object_to_json_string(object));
}

/*****************************************************************************/

void jsonrpc_add_method(char *name, jsonrpc_method method)
{
	if (!methodMap)
		methodMap = hashmap_create();
	hashmap_put(methodMap, name, method);
}

/*****************************************************************************/

void jsonrpc_system_list_methods(struct json_object *request, struct json_object *response)
{
	struct json_object *methods = json_object_new_array();
	
	if (methodMap)
	{
		char *key;
		hashmap_iterator it = hashmap_iterate(methodMap);
		while ((key = hashmap_next(&it)) != 0)
		{
			json_object_array_add(methods, json_object_new_string(key));
		}
		
		json_object_array_add(methods, json_object_new_string("system.events"));
	}
	
	json_object_object_add(response, "result", methods);
}

/*****************************************************************************/

void jsonrpc_system_events(struct json_object *request, struct json_object *response)
{
	int i;
	struct json_object *params = json_object_object_get(request, "params");
	
	int lastEvent = json_object_get_int(json_object_array_get_idx(params, 0));
	
	struct json_object *events = json_object_new_array();
	
	if (eventBus)
	{
		int count = circular_buffer_size(eventBus);
		for (i = 0; i < count; ++i)
		{
			jsonrpc_event *event = (jsonrpc_event *) circular_buffer_get(eventBus, i);
			if (lastEvent >= event->id)
				continue;
			struct json_object *jevent = json_object_new_object();
			json_object_object_add(jevent, "id", json_object_new_int(event->id));
			json_object_object_add(jevent, "type", json_object_new_string(event->type));
			json_object_object_add(jevent, "data", jsonrpc_clone_object(event->data));
			
			char timestamp[128];
			strftime(timestamp, 128, "%x %X", localtime(&(event->tm)));
			//printf("%s\n", timestamp);
			json_object_object_add(jevent, "time", json_object_new_string(timestamp));
			json_object_array_add(events, jevent);
		}
	}
	
	json_object_object_add(response, "result", events);
}


/*****************************************************************************/

char *jsonrpc_request_method(struct json_object *request)
{
	struct json_object *value;
	value = json_object_object_get(request, "method");
	return json_object_get_string(value);
}

/*****************************************************************************/

void jsonrpc_service(struct json_object *request, struct json_object *response)
{
	char *methodName = jsonrpc_request_method(request);
	
	//printf("method=%s\n", methodName);
	
	if (strcmp(methodName, "system.listMethods") == 0)
	{
		jsonrpc_system_list_methods(request, response);
	}
	else if (strcmp(methodName, "system.events") == 0)
	{
		jsonrpc_system_events(request, response);
	}
	else if (methodMap)
	{
		jsonrpc_method method = hashmap_get(methodMap, methodName);
		if (method)
		{
			(*method) (request, response);
		}
	}
}

/*****************************************************************************/

void jsonrpc_set_event_list_size(int size)
{
	if (eventBus)
		circular_buffer_free(eventBus);
	eventBus = circular_buffer_create(size);
}

/*****************************************************************************/

void jsonrpc_add_event(char *eventType, struct json_object *eventData)
{
	if (!eventBus)
		jsonrpc_set_event_list_size(100);
	
	jsonrpc_event *event = spareEvent ? spareEvent : (jsonrpc_event *) malloc(sizeof(jsonrpc_event));
	
	event->id = ++eventIds;
	event->data = json_object_get(eventData);
	event->type = (char *)malloc(strlen(eventType) + 1);
	strcpy(event->type, eventType);
	event->tm = time(NULL);
	
	spareEvent = circular_buffer_push(eventBus, event);
	
	if (spareEvent)
	{
		json_object_put(event->data);
		free(event->type);
	}
}

/*****************************************************************************/

char *_jsonrpc_process(char *request_text)
{
	char *response_text;
	struct json_object *request;
	struct json_object *response;
	
	request = json_tokener_parse(request_text);
	response = json_object_new_object();
	
	jsonrpc_service(request, response);
	
	char *text = json_object_to_json_string(response);
	response_text = malloc(strlen(text) + 1);
	strcpy(response_text, text);
	
	json_object_put(request);
	json_object_put(response);
	
	return response_text;
}

#if 0
void getBuildings(struct json_object *request, struct json_object *response)
{
	json_object_object_add(response, "result", json_object_new_string("buildings"));
}

int main(int argc, char **argv)
{
	char* request1 = "{\"method\": \"system.listMethods\", \"params\": []}";
	mc_set_debug(1);
	
	jsonrpc_add_method("controller.buildings", getBuildings);
	char* response1 = jsonrpc_process(request1);
	printf("response=%s\n", response1);
	
	char* request2 = "{\"method\": \"controller.buildings\", \"params\": []}";
	char* response2 = jsonrpc_process(request2);
	printf("response=%s\n", response2);

	return 0;
}
#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs ts=8 sw=8 noexpandtab
*/
