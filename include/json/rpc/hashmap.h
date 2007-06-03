/*
 * Simple hashmap implementation using rehash and hash algorithms from Java HashMap.
 *
 * This map is not thread safe.
 *
 * @author Marc G. Smith
 */
#ifndef __HASHMAP_H__
#define __HASHMAP_H__

/*
 * Reference to hashmap instance.
 */
typedef void *hashmap;

/*
 * Reference to hashmap iterator.
 */
typedef void *hashmap_iterator;

/*
 * Typedef for hashmap data.
 */
typedef void *hashmap_any;

/*
 * Instantiate and initialis anempty hashmap
 */
hashmap hashmap_create();

/*
 * Free resource used by hashmap. Values and keys stored in the map
 * are not freed by this method.
 * @param m the hashmap reference to free.
 */
void hashmap_free(hashmap m);

/*
 * Get value in hashmap for key.
 * @param m the hashmap reference.
 * @param key the key to lookup on.
 * @return the value stored or NULL if no match found.
 */
hashmap_any hashmap_get(hashmap m, char *key);

/*
 * Put a value in the hashmap for key.
 * @param m the hashmap reference.
 * @param key the key to lookup on.
 * @param value the value to store on key.
 * @return the old value if exists or NULL if no match found.
 */
hashmap_any hashmap_put(hashmap m, char *key, hashmap_any value);

/*
 * Get an iterator into keys the map. Ordering is arbitrary.
 * @param map the hashmap reference.
 * @return An iterator reference.
 */
extern hashmap_iterator hashmap_iterate(hashmap map);

/**
 * Get the value at the iterator and move the iterator on to the 
 * next key. The return value will be NULL when iteratation is complete
 * @param iterator reference to iterator pass in address of iterator.
 * @return a key or NULL if no more elements.
 */
extern char *hashmap_next(hashmap_iterator * iterator);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs ts=8 sw=8 noexpandtab
 */
