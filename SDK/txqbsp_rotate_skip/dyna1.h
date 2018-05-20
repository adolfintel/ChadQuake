
#ifndef _dyna1_h
#define _dyna1_h 1


#define SetupArray(type, data, max)                                 \
   type *data

#define CreateArray(type, data, max)                                \
   if (!data) (void*)data = SetupDynArray(sizeof(type), max, #max)

#define headersize                32

typedef struct
{
   int count, base_size, max_count, incr;
   char *max_name;
} arrayheader_t;

void *SetupDynArray(int base, int max, char *name);
void ExtendDynArray(arrayheader_t *data, int ncount);
void ClearArray(void *array);
void AddBytes(int Size);
void SubBytes(int Size);

extern int	total_used_memory_peak;

#ifndef _MSC_VER
inline
#endif
void ExtendArray(void *array, int ncount);

void NeedArrayBytes(void *array, int size);

#endif
