
#include <stdio.h>
#include <windows.h>
#include "bsp5.h"
#include "dyna1.h"

#define DYNARRAY_HIFACTOR	  4
#define DYNARRAY_INCREMENT_MIN    8192
#define DYNARRAY_INITIAL_MAX      4096

int	 total_used_memory;
int	 total_used_memory_peak;

void VirtualCommit(void *data, int size)
{
    void *result = VirtualAlloc(data, size, MEM_COMMIT, PAGE_READWRITE);

    if (!result)
	Message (MSGERR, "Sorry, I'm running out of memory...");

    if (result != data)
	Message (MSGERR, "VirtualAlloc : consistency error");
}


void *SetupDynArray(int base, int max, char *name)
{
    arrayheader_t *data;
    int		  Size;

    Size = headersize + base * max;

    if (options.HiLimit)
    {
    	// Set extreme limit
    	Size *= DYNARRAY_HIFACTOR;
    	max = (Size - headersize) / base;
    }

    data = (arrayheader_t*)VirtualAlloc(NULL, Size, MEM_RESERVE, PAGE_READWRITE);

//printf ("SetupDynArray %d %d %s\n", base, max, name);

    if (!data)
    {
	printf ("Cannot start the program - not enough memory addresses in the process !\n");
	exit(1);
    }

    VirtualCommit(data, DYNARRAY_INITIAL_MAX);
    memset(data, 0, DYNARRAY_INITIAL_MAX); // Make sure it's cleared

    data->count = (DYNARRAY_INITIAL_MAX-headersize) / base;
    data->base_size = base;
    data->max_count = max;
    data->max_name = name;
    data->incr = DYNARRAY_INCREMENT_MIN / base;

    total_used_memory += data->base_size * data->count;

    if (total_used_memory > total_used_memory_peak)
        total_used_memory_peak = total_used_memory;

    AddBytes(data->base_size * data->count);

    return headersize+(char*)data;
}


void ExtendDynArray(arrayheader_t *data, int ncount)
{
    int nextcount;

//printf ("ExtendDynArray %d %s... ", ncount, data->max_name);

    // ncount may be equal to data->count here; make sure limit is raised then also
    nextcount = (ncount / data->incr + 1) * data->incr;

    if (nextcount > data->max_count)
	nextcount = data->max_count;

//logprintf("ncount=%6d, next=%6d, dcnt=%6d, dmax=%8d (%s)\n", ncount, nextcount, data->count, data->max_count, data->max_name);
    if (ncount >= nextcount)
	Message (MSGERR, "Reached the limit %s (%d), %s", data->max_name, data->max_count, options.HiLimit ? "cannot continue..." : "use option \"-hilimit\"");

    ncount = nextcount;

    VirtualCommit(data, headersize+data->base_size*ncount);
    memset((char *)data + headersize + data->base_size * data->count, 0, data->base_size * (ncount - data->count)); // Make sure it's cleared

    total_used_memory += data->base_size * (ncount - data->count);

    if (total_used_memory > total_used_memory_peak)
        total_used_memory_peak = total_used_memory;

    AddBytes(data->base_size * (ncount - data->count));

    data->count = ncount;
//printf ("%d Ok\n", ncount);
}


void ClearArray(void *array)
{
    arrayheader_t header;
    arrayheader_t *data = (arrayheader_t*)((byte*)array-headersize);

//printf ("ClearArray %s\n", data->max_name);
    if (data->count > (DYNARRAY_INITIAL_MAX - headersize) / data->base_size)
    {
	memcpy(&header, data, sizeof(arrayheader_t));

	VirtualFree(data, data->base_size*data->count, MEM_DECOMMIT);
	VirtualCommit(data, DYNARRAY_INITIAL_MAX);

	total_used_memory -= header.base_size * header.count;
	SubBytes(header.base_size * header.count);

	header.count = (DYNARRAY_INITIAL_MAX-headersize) / header.base_size;

	total_used_memory += header.base_size * header.count;
	AddBytes(header.base_size * header.count);

	memcpy(data, &header, sizeof(arrayheader_t));
    }

    memset(array, 0, DYNARRAY_INITIAL_MAX - headersize); // Make sure it's cleared
}

void ExtendArray(void *array, int ncount)
{
    arrayheader_t *data = (arrayheader_t*)((byte*)array-headersize);

    // ncount really means ncount+1 here, i.e. we need space for object array[ncount]
    if (data->count <= ncount)
	ExtendDynArray(data, ncount);
}

void NeedArrayBytes(void *array, int size)
{
    arrayheader_t *data = (arrayheader_t*)((byte*)array-headersize);

    if (size)
	ExtendArray(array, (size-1) / data->base_size);
}

