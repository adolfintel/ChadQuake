
#ifndef __MEMSTICK_H__
#define __MEMSTICK_H__

// This isn't done until it becomes a flat array.

#if 0
	#define return_start(_type) memstick_t *_temps = NULL; _type _returner;
	#define maketemp(x) memstick_add (&temps, x)
	#define return_clean(x) _returner = (x); goto cleanup;
	#define return_end \
	cleanup: memstick_purge (&_temps); \
		return _returner;
#endif

STICK_TYPE_DEFINITION(floatstick_t, float);
typedef stick_t memstick_t;
void floatstick_add (floatstick_t **pstick, float floatval);


void *memstick_add (memstick_t **pstick, void *ptr);
void *memstick_purge (memstick_t **pstick);

// ONCE AND FOR ALL
void memblock_realloc_n (void *pp /* supposed to be ** !! */, int addbytes, int curbytes, int *alloced_bytes, int *alloced_blocks, int blocksize);

#endif // ! __MEMSTICK_H__



