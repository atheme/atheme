#include "json/rpc/circular_buffer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*****************************************************************************/

typedef struct circular_buffer_obj
{
	int start;
	int count;
	int size;
	int capacity;
	circular_buffer_any *table;
	
} circular_buffer_obj, *circular_buffer_ref;

/*****************************************************************************/

circular_buffer circular_buffer_create(int size)
{
	circular_buffer_ref buffer = malloc(sizeof(circular_buffer_obj));
	buffer->capacity = size;
	buffer->size = size;
	buffer->start = 0;
	buffer->count = 0;
	
	buffer->table = (circular_buffer_any *) malloc(sizeof(circular_buffer_any) * size);
	
	return (circular_buffer) buffer;
}

/*****************************************************************************/

void circular_buffer_free(circular_buffer b)
{
	circular_buffer_ref buffer = (circular_buffer_ref) b;
	free(buffer->table);
	free(buffer);
}

/*****************************************************************************/

circular_buffer_any circular_buffer_push(circular_buffer b, circular_buffer_any value)
{
	circular_buffer_ref buffer = (circular_buffer_ref) b;
	circular_buffer_any last = NULL;
	
	if (buffer->count == buffer->size)
	{
		last = (circular_buffer_any) buffer->table[buffer->start];
		buffer->table[buffer->start] = value;
		buffer->start = (buffer->start + 1) % buffer->size;
	}
	else
	{
		int index = (buffer->start + buffer->count) % buffer->size;
		buffer->table[index] = value;
		buffer->count++;
	}

	
	return last;
}

/*****************************************************************************/

circular_buffer_any circular_buffer_get(circular_buffer b, int pos)
{
	circular_buffer_ref buffer = (circular_buffer_ref) b;
	if (pos > -1 && pos < buffer->count)
	{
		int index = (buffer->start + pos) % buffer->size;
		return buffer->table[index];
	}
	
	return NULL;
}

/******************************************************************************/

int circular_buffer_size(circular_buffer b)
{
	circular_buffer_ref buffer = (circular_buffer_ref) b;
	return buffer->count;
}

/******************************************************************************/

void circular_buffer_print(circular_buffer m)
{
	int i;
	int count = circular_buffer_size(m);
	for (i = 0; i < count; ++i)
	{
		char *val = (char *)circular_buffer_get(m, i);
		printf("%s,", val);
	}
	
	printf("\n");
}

/******************************************************************************/

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs ts=8 sw=8 noexpandtab
*/
