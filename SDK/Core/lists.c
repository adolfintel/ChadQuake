/*
Copyright (C) 2009-2013 Baker

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
// lists.c

#define CORE_LOCAL
#include "core.h"
#include "lists.h" // Courtesy


// By design List_Add does not allow duplicates!
cbool List_Addf (clist_t** headnode, const char *fmt, ...) // __core_attribute__((__format__(__printf__,2,3)))
{
	cbool result;
	VA_EXPAND_ALLOC (text, length, bufsiz, fmt);
	
	result = List_Add (headnode, text);
	free (text); // free VA_EXPAND_ALLOC allocation.
	return result;
}

cbool List_Add (clist_t** headnode, const char *name)
{
	clist_t	*listent,*cursor,*prev;

	// ignore duplicate
	for (listent = *headnode; listent; listent = listent->next)
	{
		if (!strcmp (name, listent->name))
			return false;
	}

	listent = ZeroAlloc (listent); // Important now that we have extra potential pointers.

	listent->name = core_strdup (name);
	String_Edit_To_Lower_Case (listent->name);

	//insert each entry in alphabetical order
    if (*headnode == NULL ||
	    strcasecmp(listent->name, (*headnode)->name) < 0) //insert at front
	{
        listent->next = *headnode;
        *headnode = listent;
    }
    else //insert later
	{
        prev = *headnode;
        cursor = (*headnode)->next;
		while (cursor && (strcasecmp(listent->name, cursor->name) > 0))
		{
            prev = cursor;
            cursor = cursor->next;
        }
        listent->next = prev->next;
        prev->next = listent;
    }
	return true;
}


clist_t *List_Clone (const clist_t *headnode, const char *wild_patterns)
{
	clist_t *list_out = NULL;
	const clist_t *cur;

	for (cur = headnode; cur; cur = cur->next) {
		if (wild_patterns && !wildcompare (cur->name, wild_patterns))
			continue; // Disqualified

		List_Add (&list_out, cur->name);
	}
	return list_out;
}


// Similar to _joindupfz
char *List_To_String_Alloc (const clist_t *headnode, const char *separator, const char *wild_patterns)
{
	char *text = NULL; // text_a

	const clist_t *cur;

	for (cur = headnode; cur; cur = cur->next) {
		if (wild_patterns && !wildcompare (cur->name, wild_patterns))
			continue; // Disqualified

		if (separator && text)	txtcat (&text, "%s", separator);
		if (1)					txtcat (&text, "%s", cur->name);
	}

	if (!text) text = strdup (""); // Don't return NULL even if no parameters.
	return text;
}


// By design List_Add does not allow duplicates!
clist_t *List_Add_No_Case_To_Lower (clist_t **headnode, const char *name)
{
	clist_t	*listent,*cursor,*prev;

	// ignore duplicate
	for (listent = *headnode; listent; listent = listent->next)
	{
		if (!strcmp (name, listent->name))
			return NULL;
	}

	listent = ZeroAlloc (listent); // Important now that we have extra pointers in struct that are optional
	listent->name = core_strdup (name);
	//String_Edit_To_Lower_Case (listent->name);

	//insert each entry in alphabetical order
    if (*headnode == NULL ||
	    strcasecmp(listent->name, (*headnode)->name) < 0) //insert at front
	{
        listent->next = *headnode;
        *headnode = listent;
    }
    else //insert later
	{
        prev = *headnode;
        cursor = (*headnode)->next;
		while (cursor && (strcasecmp(listent->name, cursor->name) > 0))
		{
            prev = cursor;
            cursor = cursor->next;
        }
        listent->next = prev->next;
        prev->next = listent;
    }
	return listent;
}


cbool List_Find (const clist_t *headnode, const char *s_find)
{
	const clist_t *cur;
	int count;
	int result;

	for (cur = headnode, count = 0; cur; cur = cur->next)
	{
		result = strcasecmp (cur->name, s_find);
		if (result == 0)
			return true;
		if (result > 0)
			break;
	}
	return false;
}

// Depends on list being sorted
clist_t *List_Find_Item (const clist_t *headnode, const char *s_find)
{
	const clist_t *cur;
	int count;
	int result;

	for (cur = headnode, count = 0; cur; cur = cur->next)
	{
		result = strcasecmp (cur->name, s_find);
		if (result == 0)
			return (clist_t *)cur; // Returns non-const
		if (result > 0)
			break;
	}
	return NULL;
}


void List_Add_Raw_Unsorted (clist_t** headnode, const void *buf, size_t bufsiz)
{
// Black sheep function.  I think it is used to store memory blocks that might contain null characters, or not contain null characters (hence no termination)
// List_Find won't work for that reason.
	clist_t	*listent;
	
	listent = ZeroAlloc (listent);

	listent->name = (char *)core_memdup (buf, bufsiz + 1); // +1 let's at least be able to sort of see the contents as a string.
	
	// Let's at least be able to see string (barring nulls in the data).  
	// So a +1 size and NULL it so viewing as string at least isn't a buffer overrun if it has no nulls.
	listent->name[bufsiz] = 0; 
	listent->next = NULL;

	if (*headnode)
	{
		clist_t *cur;
		for (cur = *headnode; cur->next; cur = cur->next);
		cur->next = listent;

	} else *headnode = listent;

}

void List_Add_Unsorted (clist_t** headnode, const char *name)
{
	clist_t	*listent;
	listent = ZeroAlloc (listent);
	listent->name = core_strdup (name);
	listent->next = NULL;
	String_Edit_To_Lower_Case (listent->name);

	if (*headnode)
	{
		clist_t *cur;
		for (cur = *headnode; cur->next; cur = cur->next);
		cur->next = listent;

	} else *headnode = listent;
}

void List_Concat_Unsorted (clist_t** headnode, clist_t *list2)
{
	clist_t *cur, *prev = NULL;
	// Find the trailer
	for (cur = *headnode; cur; prev = cur, cur = prev->next);
	
	if (prev)
		prev->next = list2;
	else *headnode = list2;
}


void List_Free (clist_t** headnode)
{
	while (*headnode)
	{
		clist_t *item = *headnode;	// Get item

		*headnode = item->next;		// Advance before we start nuking data.
		
		freenull (item->name);
		freenull (item->extradata);
		freenull (item->extradata2);
		freenull (item);
	}

	// headnode is NULL
}


int List_Count (const clist_t *headnode)
{
	const clist_t *cur;
	int count;

	for (cur = headnode, count = 0; cur; cur = cur->next, count ++);

	return count;
}

clist_t *List_Item_Num (const clist_t *headnode, int num)
{
	const clist_t *cur;
	int count;

	for (cur = headnode, count = 0; cur; cur = cur->next, count ++)
		if (count == num)
			return (clist_t *)cur;

	return NULL; // Failure
}



int List_Compare (const clist_t *list1, const clist_t *list2)
{
	const clist_t *cur_1 = list1;
	const clist_t *cur_2 = list2;
	int count = 1;  // Notice we start at 1.  So a 1 member list returns 1 instead 0 for the comparison fail.

	while (1)
	{
		if ( !cur_1 != !cur_2 ) // What an ugly line!!  Basically if one is null and the other isn't, this fails.
			return count; // Failed comparison @ count

		if (!cur_1 || !cur_2)	// They should both have to be null because of above statement.
			return 0;			// Comparison same!

		if (strcmp (cur_1->name, cur_2->name))
			return count;		// Strings don't match.
		
		cur_1 = cur_1->next;
		cur_2 = cur_2->next;
		count ++;
	}
	return count;
}



void List_Print (const clist_t *headnode, printline_fn_t my_printline)
{
	const clist_t *cur;
	int count;
	for (cur = headnode, count = 0; cur; cur = cur->next, count ++)
		my_printline ("%d: %s", count, cur->name);
}


clist_t *List_String_Split_Alloc (const char *s, int ch_delimiter)
{
	clist_t *list = NULL;
	const char* linestart = s;
	const char* cursor = s;
	char *s2;
	size_t length =  0;

	for (linestart = s, cursor = s, length = 0; *cursor; cursor ++)
	{
		if (*cursor == ch_delimiter)
		{
			s2 = strndup (linestart, length);
			List_Add (&list, s2);
			free (s2);

			linestart = cursor + 1;
			length = 0;
		} else length++;
	}

	// Get the trailer
	s2 = strndup (linestart, length);
	List_Add (&list, s2);
	free (s2);
	
	return list;
}

//  Carriage return killer
// Aewesome 09192015
clist_t *List_String_Split_NewLines_Scrub_CarriageReturns_Alloc (const char *s, int s_len)
{
	clist_t *top = NULL, *prev_item = NULL, *item = NULL;

	const char *cursor, *linestart;
	const char *s_last_char = s_len ? &s[s_len - 1] : NULL;
	int linenum = 0;
	int line_length;
	cbool end_of_string = false;

	for (cursor = s, linestart = cursor, line_length = 0; /*nothing !! */; cursor++, line_length ++)
	{
		if (s_len && cursor > s_last_char)
			end_of_string = true;
		else {
			switch (*cursor) {
			default:			continue;									// Not either, keep going ...
			case_break '\0':	end_of_string = true;						// Null terminator hit.  Reduce count by 1.
			case_break '\n':	if (linestart[line_length - 1] == '\r')		// Newline hit.  If carriage return, back it off.
									line_length --;
			}
		}

		// If null is first line (top = NULL), write it.  Even if zero length, because file does exist.
		// If not the first line, if we hit null terminator and length is zero do not write.

		if (end_of_string && line_length == 0 && top != NULL)
			break; // Get out without writing, there is nothing to write.

		item = ZeroAlloc(item);
		item->name = strndup (linestart, line_length);
		item->next = NULL;

		//logd ("Wrote line: '%s'", item->name);
		linenum ++;
//		alert ("#%03d: %s", linenum, item->name);

		if (prev_item)	prev_item->next = item;
		if (!top)		top = item;
		prev_item = item;

		if (end_of_string)
			break; // We had something in the buffer when we hit end and needed to write it out, but now that is complete so get out.

		linestart = &cursor[1], line_length = -1;
	}
	// Last one


	return top;
}


// Will add a trailing empty line after a newline.
clist_t *List_From_String_Lines_Alloc (const char *s)
{
	clist_t *out = NULL;
	char *buf = core_strdup (s); // Copy
	
	char *ch, *thisline;
	size_t count;

	for (ch = buf, thisline = ch, count = 0; *ch; ch++, count++)
	{
		if (*ch != '\n')
			continue;

		// New line char
		*ch = 0; // Null it out
		String_Edit_Whitespace_To_Space (thisline);
		String_Edit_Trim (thisline);

		List_Add_Unsorted (&out, thisline);

		thisline = &ch[1]; // Next character
		count = 0;
	}
	
	String_Edit_Whitespace_To_Space (thisline);
	String_Edit_Trim (thisline);
	List_Add_Unsorted (&out, thisline);

	buf = core_free (buf); // Free buffer
	return out;
}

