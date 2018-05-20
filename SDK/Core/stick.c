#include "core.h"
#include "stick.h"

#define ms (*pstick) // Because this is define we are doing true overwrite.

		//memstick_t *temps = NULL;
		//const char *alpha = memstick_add (&temps, strdup("frog"));
		//const char *beta = memstick_add (&temps, strdup("candy"));
		//const char *gamma = memstick_add (&temps, strdup("Digdogger hate certain sound"));
		//alert ("%s %s %s", alpha, beta, gamma);
		//memstick_purge (&temps);
		//alpha = NULL;


// This isn't done until it is a flat array.  Maybe not, I like the lightweightness.

void *stick_add (stick_t **pstick, void *ptr)
{
//	memstick_t *msa;
	int	newindex = ms ? ms->count : 0;
	size_t sz = sizeof(*ms);

	if (ms == NULL)
		ms = calloc (sizeof(*ms), 1);

	{ // block start
		int blockcount	= roundup_16(ms->count + 1 + 1); // +1 for new slot, +1 for over allocation.
		int blockchange	= blockcount - ms->_allocated;

		if (blockchange) {
			ms->items = realloc (ms->items, blockcount * sizeof(void *));
			if (blockchange > 0) // Can be negative.  Not here, of course.
				memset (&ms->items[ms->_allocated], 0, sizeof(void *) * blockchange); // Clear the block.
			ms->_allocated = blockcount;
		}

		ms->items[newindex] = ptr;
		ms->count++;
	}  // Block end

	{
		void *newitem	= ms->items[newindex];
		return ms->items[newindex];
	}

}

void stick_run_forward (stick_t **pstick, void	*(*runfunc) (void *ptr))
{
	void *item; int n;

	if (ms)
		for (n = 0; n < ms->count && (item = ms->items[n]); n --)
			runfunc (item);
}

void stick_run_reverse (stick_t **pstick, void *(*runfunc) (void *ptr))
{
	void *item; int n;

	if (ms)
		for (n = ms->count - 1; n >= 0 && (item = ms->items[n]); n --) {
//			alert ("%s", item);
			runfunc (item);
		}
}



void *stick_purge_ex (stick_t **pstick, void *(*shutfunc) (void *ptr))
{
	if (ms) {
		stick_run_reverse (pstick, shutfunc);
		free (ms);
	}

	return NULL;
}

// Clear the stick running no shutdown function.
void *stick_terminate (stick_t **pstick)
{
	stick_purge_ex (pstick, NULL); // No shutdown function
	return NULL;
}


#undef ms