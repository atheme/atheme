/*
 * Simple circular_buffer implementation using rehash and hash algorithms from Java HashMap.
 *
 * This map is not thread safe.
 *
 * @author Marc G. Smith
 */
#ifndef __CIRCULAR_BUFFER_H__
#define __CIRCULAR_BUFFER_H__

/*
 * Reference to circular_buffer instance.
 */
typedef void *circular_buffer;

/*
 * Reference to circular_buffer iterator.
 */
typedef void *circular_buffer_iterator;

/*
 * Typedef for circular_buffer data.
 */
typedef void *circular_buffer_any;

/*
 * Instantiate and initialis anempty circular_buffer
 */
circular_buffer circular_buffer_create(int size);


/*
 * Free resource used by circular_buffer. Values and keys stored in the map
 * are not freed by this method.
 * @param m the circular_buffer reference to free.
 */
void circular_buffer_free(circular_buffer m);


/*
 * Put a value in the circular_buffer for key.
 * @param m the circular_buffer reference.
 * @param value the value to store on key.
 * @return the old value if exists or NULL if no match found.
 */
circular_buffer_any circular_buffer_push(circular_buffer m, circular_buffer_any value);

circular_buffer_any circular_buffer_get(circular_buffer m, int pos);

int circular_buffer_size(circular_buffer m);

/*
 * Get an iterator into values. 
 * @param map the circular_buffer reference.
 * @return An iterator reference.
 */
extern circular_buffer_iterator circular_buffer_iterate(circular_buffer map);

/**
 * Get the value at the iterator and move the iterator on to the 
 * next key. The return value will be NULL when iteratation is complete
 * @param iterator reference to iterator pass in address of iterator.
 * @return a key or NULL if no more elements.
 */
extern char *circular_buffer_next(circular_buffer_iterator * iterator);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs ts=8 sw=8 noexpandtab
 */
