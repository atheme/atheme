#include "json/rpc/hashmap.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*****************************************************************************/


struct hashmap_obj;

/*****************************************************************************/

typedef struct hashmap_entry
{
	char *key;
	hashmap_any value;
	int index;
	int hash;
	struct hashmap_entry *next;
	struct hashmap_obj *map;
} hashmap_entry;

/*****************************************************************************/

typedef struct hashmap_obj
{
	float loadFactor;
	int initialCapacity;
	int count;
	int threshold;
	int size;
	int tableSize;
	struct hashmap_entry **table;
	
} hashmap_obj, *hashmap_ref;

/*****************************************************************************/

hashmap hashmap_create()
{
	int i;
	hashmap_ref map = malloc(sizeof(hashmap_obj));
	map->initialCapacity = 101;
	map->tableSize = map->initialCapacity;
	map->loadFactor = 0.75f;
	map->table = malloc(sizeof(hashmap_entry *) * map->tableSize);
	map->threshold = (int)(map->initialCapacity * map->loadFactor);
	
	for (i = 0; i < map->tableSize; ++i)
	{
		map->table[i] = 0;
	}
	
	return (hashmap) map;
}

/*****************************************************************************/

void hashmap_free(hashmap m)
{
	int i;
	hashmap_entry *e;
	hashmap_ref map = (hashmap_ref) m;
	
	for (i = 0; i < map->tableSize; ++i)
	{
		for (e = map->table[i]; e;)
		{
			hashmap_entry *c = e;
			e = c->next;
			free(c);
		}
	}
	
	free(map->table);
	free(map);
}

/*****************************************************************************/

int hashmap_get_hash(char *key)
{
	int i;
	int h = 0;
	
	int off = 0;
	int len = strlen(key);
	
	if (len < 16)
	{
		for (i = len; i > 0; i--)
		{
			h = (h * 37) + key[off++];
		}
	}
	else
	{
		// only sample some characters
		int skip = len / 8;
		for (i = len; i > 0; i -= skip, off += skip)
		{
			h = (h * 39) + key[off];
		}
	}
	
	return h;
	
}

/*****************************************************************************/

void hashmap_rehash(hashmap_ref map)
{
	int i;
	hashmap_entry *old;
	int oldCapacity = map->tableSize;
	hashmap_entry **oldMap = map->table;
	
	int newCapacity = oldCapacity * 2 + 1;
	hashmap_entry **newMap = malloc(sizeof(hashmap_entry) * newCapacity);

	map->threshold = (int)(newCapacity * map->loadFactor);
	map->table = newMap;
	map->tableSize = newCapacity;
	
	for (i = 0; i < map->tableSize; ++i)
	{
		map->table[i] = 0;
	}
	
	for (i = oldCapacity; i-- > 0;)
	{
		for (old = oldMap[i]; old;)
		{
			int index;
			hashmap_entry *e = old;
			old = old->next;
			
			index = (e->hash & 0x7FFFFFFF) % newCapacity;
			e->next = newMap[index];
			e->index = index;
			newMap[index] = e;
		}
	}
	
	free(oldMap);
}

/*****************************************************************************/

hashmap_any hashmap_get(hashmap m, char *key)
{
	hashmap_ref map = (hashmap_ref) m;
	hashmap_entry *e;
	hashmap_entry **table = map->table;
	
	int hash = hashmap_get_hash(key);
	int index = (hash & 0x7FFFFFFF) % map->tableSize;
	for (e = table[index]; e; e = e->next)
	{
		if ((e->hash == hash) && strcmp(key, e->key) == 0)
		{
			return e->value;
		}
	}
	
	return 0;
}

/*****************************************************************************/

hashmap_any hashmap_put(hashmap m, char *key, hashmap_any value)
{
	hashmap_ref map = (hashmap_ref) m;
	hashmap_entry *e;
	hashmap_entry **table = map->table;
	
	int hash = hashmap_get_hash(key);
	int index = (hash & 0x7FFFFFFF) % map->tableSize;
	for (e = table[index]; e; e = e->next)
	{
		if ((e->hash == hash) && strcmp(key, e->key) == 0)
		{
			void *old = e->value;
			e->value = value;
			return old;
		}
	}
	
	if (map->count >= map->threshold)
	{
		// Rehash the table if the threshold is exceeded
		hashmap_rehash(map);
		table = map->table;
		index = (hash & 0x7FFFFFFF) % map->tableSize;
	}
	
	// Creates the new entry.
	e = malloc(sizeof(hashmap_entry));
	e->hash = hash;
	e->key = key;
	e->value = value;
	e->next = table[index];
	e->index = index;
	e->map = map;
	
	map->count++;
	map->table[index] = e;
	
	return 0;
}

/*****************************************************************************/

hashmap_iterator hashmap_iterate(hashmap m)
{
	int i;
	hashmap_ref map = (hashmap_ref) m;
	
	for (i = 0; i < map->tableSize; ++i)
	{
		if (map->table[i])
		{
			return (hashmap_iterator) map->table[i];
		}
	}
	
	return 0;
}

/******************************************************************************/

char *hashmap_next(hashmap_iterator * iterator)
{
	int i;
	hashmap_entry *e = (hashmap_entry *) * iterator;
	char *key = 0;
	
	if (e)
	{
		hashmap_ref map = e->map;
		key = e->key;
		
		if (e->next)
		{
			e = e->next;
		}
		else
		{
			for (i = e->index + 1; i < map->tableSize; ++i)
			{
				if ((e = map->table[i]))
				{
					break;
				}
			}
			
		}
		
		*iterator = e;
	}
	
	return key;
}

#if 0
int main(int argc, char **argv)
{
	hashmap map = hashmap_create();
	hashmap_put(map, "one", "One");
	hashmap_put(map, "two", "Two");
	hashmap_put(map, "three", "Three");
	hashmap_put(map, "four", "Four");
	
	char* key;
	hashmap_iterator it = hashmap_iterate(map);
	while((key = hashmap_next(&it)) != 0)
	{
		char* value = (char*) hashmap_get(map, key);
		printf("key=%s, value=%s\n", key, value);
	}
	
	hashmap_put(map, "one", "OneOne");
	hashmap_put(map, "two", "TwoTwo");
	hashmap_put(map, "three", "ThreeThree");
	hashmap_put(map, "four", "FourFour");
	hashmap_put(map, "zero", "ZeroZero");
	
	it = hashmap_iterate(map);
	while((key = hashmap_next(&it)) != 0)
	{
		char* value = (char*) hashmap_get(map, key);
		printf("key=%s, value=%s\n", key, value);
	}
	
	hashmap_free(map);
	return 0;
}
#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs ts=8 sw=8 noexpandtab
 */
