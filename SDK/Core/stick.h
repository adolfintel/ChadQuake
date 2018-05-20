#ifndef __STICK_H__
#define __STICK_H__

// stick.h - a lightweight magic stick that is basically an "add-only" array.
//           

/*
typedef struct {
	int		_allocated;	 // count + 1 // rowbytes sizeof(void *)
	int		count;
	void	**items; // List of allocations
} stick_t;
*/

#define STICK_TYPE_DEFINITION(Name_t, ItemType_t) \
	typedef struct { \
		int			_allocated; \
		int			count; \
		ItemType_t	**items; \
	} Name_t

STICK_TYPE_DEFINITION(stick_t, void);

void *stick_add (stick_t **pstick, void *ptr);

// Run a function against a stick forward or reverse.
void stick_run_forward (stick_t **pstick, void *(*runfunc) (void *ptr));
void stick_run_reverse (stick_t **pstick, void *(*runfunc) (void *ptr));

void *stick_terminate (stick_t **pstick);

// void *stick_purge_ex (memstick_t **pstick, void	*(*shutfunc) (void *ptr)); // So memstick can call this using core_free.
// But this isn't 




#endif // __STICK_H__