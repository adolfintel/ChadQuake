/*
Copyright (C) 2013-2014 Baker

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// utilities_install.c -- Baker: utilities with a layer of separation


#ifdef QUAKE_GAME
#include "quakedef.h"
#include "utilities.h"
#endif // QUAKE_GAME

void Install_Init (void)
{

}
#if 0

/*
typedef struct
{
	char * archive
	char * title
	char *author
	char *date
	char *size
	char *commandline // Check for -quoth
	char *rating
	char *md5sum
}
*/
typedef struct sheet_x_s
{
	int numcolumns;
	int columnsize; //sizeof(char *) * columns
	int rows;
	
//	char **columns; // Alloc columns = sizeof(char *) * columns;
	char **rows; // columnsize * rows;  realloc. // Row 0 is header.
	char **active;
} sheet_x_t;

sheet_x_t *Sheet_Free (sheet_x_t *sht)
{
	int num = (sht->numrows + 1) * sht->numcolumns, n;
	for (n = 0; n < num; n ++) // // Free every column in every row.
		sht->row[n] = core_free (sht->row[n]);

	sht->rows = core_free(sht->rows); // // Then free the rows
	return (sht = core_free (sht));  // Then free the sheet and return NULL
}

void Sheet_Print (sheet_x_t *sht, const char *colsep, const char *rowsep)
{
	int r, c;
	for (r = 0; r < sht->numrows + 1; r ++)
	{
		for (c = 0; c < sht->numcolumns; c ++)
		{
			if (c) printf (colsep);
			printf ("%s", sht->row[r * c]);
		}
		printf (rowsep);
		if (!r) printf ("====================== %s", rowsep);
	}
}


sheet_x_t *Sheet_Alloc (int count, ... )
{
	sheet_x_t *sht = core_calloc (sizeof(columnar_t), 1);

	sht->numcolumns = count;
	sht->columnsize = count * sizeof (char *);
	sht->numrows = 0; // Don't have any yet.
	//colx->columns = core_calloc (colx->columnsize);
	sht->rows = core_calloc (sht->columnsize);
	sht->active = &sht->rows[0 * sht->columnsize]; // columnsize * rows;  realloc. // Row 0 is header.
	{
		va_list ap;
		char *s;
		int n;

		va_start (ap, count);         /* Initialize the argument list. */
		for (n = 0; n < count; n ++)
		{
			s = va_arg (ap, char *);    /* Get the next argument value. */
			//printf ("%d: %s", n, s);
			sht->active[n] = core_strdup (s);
		}

		va_end (ap);                  /* Clean up. */
	}

	return sht;
}


sheet_x_t *Sheet_Alloc (int count, ... )
{
	sheet_x_t *sht = core_calloc (sizeof(columnar_t), 1);

	sht->numcolumns = count;
	sht->columnsize = count * sizeof (char *);
	sht->numrows = 0; // Don't have any yet.
	//colx->columns = core_calloc (colx->columnsize);
	sht->rows = core_realloc (sht->columnsize, sht->numrows + 1);
	sht->active = &sht->rows[sht->numrows /*0*/ * sht->columnsize]; // columnsize * rows;  realloc. // Row 0 is header.
	{
		va_list ap;
		char *s;
		int n;

		va_start (ap, count);         /* Initialize the argument list. */
		for (n = 0; n < count; n ++)
		{
			s = va_arg (ap, char *);    /* Get the next argument value. */
			//printf ("%d: %s", n, s);
			sht->active[n] = core_strdup (s);
		}

		va_end (ap);                  /* Clean up. */
	}

	return sht;
}

void Sheet_Add (sheet_x_t *sht, ... )
{
	sht->numrows++;
	sht->rows = core_realloc (sht->rows, sht->columnsize * sht->numrows + 1);

	sht->active = &sht->rows[sht->numrows * sht->columnsize]; // columnsize * rows;  realloc. // Row 0 is header.
	{
		va_list ap;
		char *s;
		int n;

		va_start (ap, count);         /* Initialize the argument list. */
		for (n = 0; n < count; n ++)
		{
			s = va_arg (ap, char *);    /* Get the next argument value. */
			//printf ("%d: %s", n, s);
			sht->active[n] = core_strdup (s);
		}

		va_end (ap);                  /* Clean up. */
	}

	return sht;
}


void Install_Init (void)
{
	int size = 0, exitcode = 0;
	void *mem = Download_To_Memory_Alloc (QUAKE_INJECTOR_USER_AGENT, 
										"http://www.quaddicted.com/reviews/quaddicted_database.xml", NULL, NULL, &size, &exitcode);

	if (mem) {
		int count;
		colalloc = Something(8);

		char *archive;
		char *title;
		char *author;
		char *date;
		char *size;
		char *commandline; // Check for -quoth
		char *rating;
		char *md5sum;
		size=size;
	}
	mem = core_free (mem);
}
#endif