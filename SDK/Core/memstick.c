#include "core.h"
#include "stick.h"
#include "memstick.h"

// This isn't done until it is a flat array.  Maybe not, I like the lightweightness.

// TODO: This is weaker than I prefer, shouldn't take a memory allocation but rather the address of the holder
// However, perhaps I need to be using stringa objects though?
// Handle with care!
void *memstick_add (memstick_t **pstick, void *ptr)
{
	return stick_add (pstick, ptr);
}

void floatstick_add (floatstick_t **pstick, float floatval)
{
	float *floatcopy = MemDup(&floatval);
	stick_add ( (stick_t **)pstick, floatcopy);

}



void *memstick_purge (memstick_t **pstick)
{
	void *stick_purge_ex (stick_t **pstick, void *(*shutfunc) (void *ptr));
	typedef void *(*sheesh_fn_t) (void *ptr);
	return stick_purge_ex (pstick, (sheesh_fn_t)core_free); // Cast avoids silly warning const void vs void
}




// Mem read --- bad home but whatever


void memblock_realloc_n (void *pp /* void *pp */, int addbytes, int curbytes, int *alloced_bytes, int *alloced_blocks, int blocksize)
{
    void **p = (void **)pp; // Stupid compiler warnings.
	// 0 + 10 >= 10 - yes ... always ensure at least one byte of zero memory
	if (curbytes + addbytes >= *alloced_bytes) {
		byte **data = (byte **)p; // Yes, crazy.
		int new_blockcount	= (curbytes + addbytes) / blocksize + 1;
		int new_size		= new_blockcount * blocksize;
		int old_size		= *alloced_bytes;
		byte *writer = NULL;
		*data = realloc (*data, new_size);
		writer = *data;

		memset (&writer[old_size], 0, new_size - old_size); // 0 format new blocks

		*alloced_bytes		= new_size;
		*alloced_blocks		= new_blockcount;
//		alert ("Realloced %d to %d.  Last byte is %d (%d) and next byte is (%d)", old_size, new_size, curbytes, curbytes > 0 ? writer[curbytes - 1] : -1, writer[curbytes]);
	}
}

