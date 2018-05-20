/*
 Copyright (C) 2009-2014 Baker
 *
 *Permission to use, copy, modify, and distribute this software for any
 *purpose with or without fee is hereby granted, provided that the above
 *copyright notice and this permission notice appear in all copies.
 *
 *THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
// stringlib.c -- extended string functionality


#include "core.h"		// Barely required
#include "stringlib.h" // Courtesy

#include <stdarg.h>
#include <string.h> // strlen, etc.
#include <ctype.h> // islower, etc.

// Do not like enough for them to be widely available
#define isnotvisibleascii(c) ((c) <= 32 || 127 <=(c)) // This actually already includes 127 delete char, good job
#define iswhitespace(c) ((c) <= 32) // What about 127 delete char? // Going for the traditional definition of whitespace here I guess.  I dis
#define text_a char // More like const char, actually

// Short: Argv to command line
// Notes: None.
// Unit Test:
void String_Command_Argv_To_String (char *cmdline, int numargc, char **argvz, size_t siz)
{
// Unfortunately, this is an inexact science that will not have quotes intact because system argv removes them.
// Non-Windows deconstructs a command line into arguments
// meaning it is lossy and actually impossible to reconstruct the original.
// With 100% certainty.
// Example mybin "hello" "my friend" --> arg0 mybin arg1 hello arg2 my friend <--- if there were quotes, we'd never know!
	int i;

	cmdline[0] = 0;

	for (i = 0; i < numargc; i++)
	{
		if (!argvz[i]) // Hit a null
			break;

		if (i > 0)
			strlcat (cmdline, " ", siz);
		strlcat (cmdline, argvz[i], siz);
	}
}


// Short: Command line to argv
// Notes: None.
// Unit Test:

void String_Command_String_To_Argv (char *cmdline, int *numargc, char **argvz, int maxargs)
{
	// Baker: This converts a commandline in arguments and an argument count.
	// Requires cmd_line, pointer to argc, argv[], maxargs
	while (*cmdline && (*numargc < maxargs))
	{
#if 0
		const char *start = cmdline;
		int len;
#endif
		// Advance beyond spaces, white space, delete and non-ascii characters
		// ASCII = chars 0-127, where chars > 127 = ANSI codepage 1252
		while (*cmdline && (*cmdline <= SPACE_CHAR_32 || MAX_ASCII_PRINTABLE_126 <= *cmdline ) ) // Was 127, but 127 is DELETE
			cmdline++;

		switch (*cmdline)
		{
		case 0:  // null terminator
			break;

		case '\"': // quote

			// advance past the quote
			cmdline++;

			argvz[*numargc] = cmdline;
			(*numargc)++;

			// continue until hit another quote or null terminator
			while (*cmdline && *cmdline != '\"')
				cmdline++;
#if 0
			len = cmdline - start;
#endif
			break;

		default:
			argvz[*numargc] = cmdline;
			(*numargc)++;

			// Advance until reach space, white space, delete or non-ascii
			while (*cmdline && (SPACE_CHAR_32 < *cmdline && *cmdline <= MAX_ASCII_PRINTABLE_126  ) ) // Was < 127
				cmdline++;
#if 0
			len = cmdline - start;
#endif
		} // End of switch

		// If more advance the cursor past what should be whitespace
		if (*cmdline)
		{
			*cmdline = 0;
			cmdline++;
		}

	} // end of while (*cmd_line && (*numargc < maxargs)
}

// Short: Command line to argv
// Notes: None.
// Unit Test:

void String_To_Arg_Buckets (struct arg_buckets_64_s *argbucket, const char *s)
{
	c_strlcpy(argbucket->cmdline, s);
	String_Command_String_To_Argv (argbucket->cmdline, &argbucket->argcount, argbucket->argvs, ARG_BUCKETS_COUNT_64);
}


// Short: Counts instances of a character in string
// Notes: None.
// Unit Test:
int String_Count_Char (const char *s, int ch_findchar)
{
	const char *s_end = String_Find_End (s);

	if (s_end)
		return String_Range_Count_Char (s, s_end, ch_findchar);
	return 0;
}


// Short: Counts newline characters (char 10) in string
// Notes: None.
int String_Count_Newlines (const char *s)
{
	return String_Count_Char (s, '\n');
}


// Short: Counts instances of string in string
// Notes: None.
int String_Count_String (const char *s, const char *s_find)
{
	size_t len = strlen(s_find);
	int count;
	const char *search;
	for (search = strstr(s, s_find), count = 0; search; search = strstr(search + len, s_find))
		count ++;

	return count;
}


int String_Count_Words (const char *s)
{
    int count = 0;
    //printf ("Word is %s and  ... ", s);

    while (*s)
    {
        if (*s != SPACE_CHAR_32) {
            count ++;
            while (*s && *s != SPACE_CHAR_32)
                s ++;
        }
        else {
            while (*s && *s == SPACE_CHAR_32)
                s ++;
//            in_space = 1;
        }
    }
    //printf ("count is %d", count);
	return count;
}

int String_Count_First_Word_Length (const char *s)
{
    const char *first_space = strchr (s, ' ');
    if (!first_space) return strlen(s); // First word is everything.
    if (first_space == s) return -1; // First "word" is a space.
    return first_space - s;
}


#undef String_Does_Contain
// Short: Returns 1 if string contains string, otherwise returns 0
// Notes: For reference only, function has no advantage over strstr.
int String_Does_Contain (const char *s, const char *s_find)
{
	if (strstr (s, s_find))
		return 1;

	return 0;
}


// Short: Returns 1 if string contains spaces, otherwise returns 0
// Notes: Offers no advantage over strchr(s, SPACE_CHAR_32).
int String_Does_Contain_Spaces (const char *s)
{
	if (strchr(s, SPACE_CHAR_32))
		return 1;

	return 0;
}


// Short: Returns 1 if delimited string contains string as delimited member, otherwise returns 0
// Notes: If the string to find contains the delimiter character, the function returns 0.
int String_Does_List_Delimited_Contain (const char *s, const char *s_find, int ch_delimiter)
{
	const char	*match, *next_start;
	size_t		s_find_len;

	// Check for disqualifying situations ...
	if (s == NULL || *s == 0 || s_find == NULL || *s_find == 0 || strchr (s_find, ch_delimiter))
		return 0;

	s_find_len = strlen (s_find);

	for (match = strstr (s, s_find) ; match ; match = strstr (next_start, s_find))
	{
		next_start = match + s_find_len;

		// The beginning must be (1) the start of the string or (2) preceded by the delimiter
		if (match == s || *(match - 1) == ch_delimiter)
		{
			// The matching text must be followed by (1) null at end of string or (2) a delimiter
			if (*next_start == 0 || *next_start == ch_delimiter)
				return 1;
		}
	}

	return 0;
}


// Short: Returns 1 if string ends with the suffix -- considering case, otherwise returns 0
// Notes: None.
int String_Does_End_With (const char *s, const char *s_suffix)
{
	size_t s_len = strlen(s);
	size_t s_suffix_len = strlen (s_suffix);
	ssize_t suffix_spot = s_len - s_suffix_len;

	if (s_len && s_suffix_len && suffix_spot >= 0 )
		return !strcmp (&s[suffix_spot], s_suffix);

	return 0;
}


// Short: Returns 1 if string ends with the suffix -- ignoring case, otherwise returns 0
// Notes: None.
int String_Does_End_With_Caseless (const char *s, const char *s_suffix)
{
	size_t s_len = strlen(s);
	size_t s_suffix_len = strlen (s_suffix);
	ssize_t suffix_spot = s_len - s_suffix_len;

	if (s_len && s_suffix_len && suffix_spot >= 0 )
		return !strcasecmp (&s[suffix_spot], s_suffix);

	return 0;
}


// Short: Returns 1 if string contains lower case characters, otherwise returns 0
// Notes: None.
int String_Does_Have_Lowercase (const char *s)
{
	if (s)
	{
		const char *search;
		for (search = s ; *search ; search ++)
		{
			if (islower(*search))
				return 1;
		}
	}

	return 0;
}


// Short: Returns 1 if string begins with and ends with a quote and has length greater than 1.
// Note:  A string with length 1 where the only character was a quote would return 0.
int String_Does_Have_Quotes (const char *s)
{
	size_t len = strlen(s);
	if (len > 1 && s[0] == '\"' && s[len - 1] == '\"')
		return 1;
	return 0;
}


// Short: Returns 1 if string contains upper case characters, otherwise returns 0
// Notes: None.
int String_Does_Have_Uppercase (const char *s)
{
	if (s)
	{
		const char *search;
		for (search = s ; *search ; search ++)
		{
			if (isupper(*search))
				return 1;
		}
	}

	return 0;
}


cbool String_Is_Only_Alpha_Numeric_Plus_Charcode (const char *s, int charcode)
{
	const char *cursor;
	int ch;

	for (cursor = s; *cursor; cursor ++) {
		ch = *cursor;

		if (isdigit (ch) || isalpha (ch) || ch == charcode)
			continue;

		return false;
	}

	return true; // It's only alpha numeric plus specified character.
}


#undef String_Does_Match
// Short: Returns 1 if string is identical --- considering case, otherwise returns 0
// Notes: Offers no advantage over using !strcmp(s1,s2).
int String_Does_Match (const char *s1, const char *s2)
{
	return !strcmp(s1, s2);
}


#undef String_Does_Match_Caseless
// Short: Returns 1 if string is identical --- ignoring case, otherwise returns 0
// Notes: Offers no advantage over using !strcasecmp(s1,s2)
int String_Does_Match_Caseless (const char *s1, const char *s2)
{
	return !strcasecmp(s1, s2);
}


#undef String_Does_Match_Nullproof
// Short: Returns 1 if string is identical --- considering case --- even if one or both strings are NULL, otherwise returns 0
// Notes: If string pointers are identical, the function does not perform a byte-by-byte comparison and returns 1.
int String_Does_Match_Nullproof (const char *s1, const char *s2)
{
	return ( s1 == s2 ? 1 : ((s1 == NULL || s2 == NULL) ? 0 : !strcmp (s1, s2)));
}


#undef String_Does_Not_Match
// Short: Returns 1 if string is different --- considering case, otherwise returns 0
// Notes: Offers no advantage over strcmp(s1, s2).
int String_Does_Not_Match (const char *s1, const char *s2)
{
	return !!strcmp(s1, s2);
}


#undef String_Does_Not_Match_Caseless
// Short: Returns 1 if string is different --- ignoring case, otherwise returns 0
// Notes: Offers no advantage over strcasecmp(s1, s2).
int String_Does_Not_Match_Caseless (const char *s1, const char *s2)
{
	return !!strcasecmp(s1, s2);
}


#undef String_Does_Not_Start_With
// Short: Returns 1 if string does not begin with prefix --- considering case, otherwise returns 0
// Notes: None.
int String_Does_Not_Start_With (const char *s, const char *s_prefix)
{
	return !!strncmp(s, s_prefix, strlen(s_prefix));
}


#undef String_Does_Not_Start_With_Caseless
// Short: Returns 1 if string does not begin with prefix --- ignoring case, otherwise returns 0
// Notes: None.
int String_Does_Not_Start_With_Caseless (const char *s, const char *s_prefix)
{
	return !!strncasecmp(s, s_prefix, strlen(s_prefix));
}


#undef String_Does_Start_With
// Short: Returns 1 if string begins with prefix --- considering case, otherwise returns 0
// Notes: None.
int String_Does_Start_With (const char *s, const char *s_prefix)
{
	return !strncmp(s, s_prefix, strlen(s_prefix));
}


#undef String_Does_Start_With_Caseless
// Short: Returns 1 if string begins with prefix --- ignoring case, otherwise returns 0
// Notes: None.
int String_Does_Start_With_Caseless (const char *s, const char *s_prefix)
{
	return !strncasecmp(s, s_prefix, strlen(s_prefix));
}


// Inserts text in a buffer at offset (with buf maxsize of bufsize) and returns inserted character count
// Notes: None.
int String_Edit_Insert_At (char* s_edit, size_t s_size, const char* s_insert, size_t offset)
{
	// about our s_edit ...
	size_t buffer_maxsize = s_size - 1; // room for null
	size_t buffer_strlen = strlen (s_edit);
	size_t buffer_remaining = buffer_maxsize - buffer_strlen;	// max insertable chars
	size_t chars_after_insert_point = buffer_strlen - offset + 1; // Need to move null term

	// now factor in insertion text
	size_t insert_text_len = strlen (s_insert);
	size_t copy_length = insert_text_len > buffer_remaining ? buffer_remaining : insert_text_len;

	if (copy_length)
	{
		memmove (s_edit + offset + copy_length, s_edit + offset, chars_after_insert_point + 1);
		memcpy (s_edit + offset, s_insert, copy_length);
	}

	return (int)copy_length;
}


// Short: Removes all leading white-spaces (char < SPACE_CHAR_32) from string except for spaces (char SPACE_CHAR_32)
// Notes: None.
char *String_Edit_LTrim_Whitespace_Excluding_Spaces (char *s_edit)
{
	char *s_end = String_Find_End (s_edit);
	char *s_past_white = String_Skip_WhiteSpace_Excluding_Space (s_edit);

	memmove (s_edit, s_past_white, s_end - s_past_white + 1);

	return s_edit;
}


// Short: Removes all leading white-spaces (char <= SPACE_CHAR_32 ) from string including spaces (char SPACE_CHAR_32)
// Notes: None.
char *String_Edit_LTrim_Whitespace_Including_Spaces (char *s_edit)
{
	if (s_edit[0]) // Don't bother for 0 length string
	{
		char *s_end = String_Find_End (s_edit);
		char *s_past_white = String_Skip_WhiteSpace_Including_Space (s_edit);

		memmove (s_edit, s_past_white, (s_end - s_past_white + 1 /*range size*/) + 1 /* null term */);
	}

	return s_edit;
}


// Short: Removes all trailing white-spaces (char < SPACE_CHAR_32) from string except for spaces (char SPACE_CHAR_32) by replacing with null characters
// Notes: None.
char *String_Edit_RTrim_Whitespace_Excluding_Spaces (char *s_edit)
{
	int len = strlen(s_edit), j;

	for (j = len - 1; j >= 0; j --)
	{
		if (s_edit[j] < SPACE_CHAR_32)
			s_edit[j] = 0;
		else break;
	}

	return s_edit;
}


// Short: Removes all trailing white-spaces (char <= SPACE_CHAR_32) from string including for spaces (char SPACE_CHAR_32) by replacing with null characters
// Notes: None.
char *String_Edit_RTrim_Whitespace_Including_Spaces (char *s_edit)
{
	int len = strlen(s_edit), j;

	for (j = len - 1; j >= 0; j --)
	{
		if (s_edit[j] <= SPACE_CHAR_32)
			s_edit[j] = 0;
		else break;
	}

	return s_edit;
}

// Short: Removes last character from string by replacing it with null character
// Notes: None.
char *String_Edit_Remove_End (char *s_edit)
{
	int len = strlen(s_edit);

	if (len >= 1)
		s_edit[len-1] = 0;

	return s_edit;
}


// Short: Removes trailing spaces by replacing with null characters
// Notes: None.
char *String_Edit_RemoveTrailingSpaces (char *s_edit)
{
	ssize_t offset;
	for (offset = strlen(s_edit) - 1; offset >= 0 && s_edit[offset] == SPACE_CHAR_32; offset--)
		s_edit[offset] = 0; // remove trailing spaces

	return s_edit;
}


char *String_Edit_RemoveTrailingZeros (char *s_edit)
{
	char *cursor = strchr (s_edit, '\0');

	if (cursor && cursor > s_edit) {
		for (cursor --; cursor >= s_edit && *cursor == '0'; cursor --)
			*cursor = 0;

		if (*cursor == '.')
			*cursor = 0;
	}
	return s_edit;
}

// We leave at least 1 decimal after the dot.  Examples: 2.000 -> 2.0, 2.1000 -> 2.1
char *String_Edit_To_ProperFloat (char *s_edit)
{
	char *dot = strchr (s_edit, '.');
	char *decimal2 = dot && dot[1] && dot[2] ? &dot[2] : NULL;

	if (decimal2) {
		char *cursor = strchr (decimal2, '\0' /* like the null terminator escape 0 */);
		for (cursor-- ; cursor >= decimal2 && *cursor == '0' /* like the digit zero */; cursor --)
			*cursor = 0;
	}
	return s_edit;
}


// Short: Replaces string buffer with string repeated N times
// Notes: None.
char *String_Repeat_Alloc (char ch, int count)
{
	char *out = calloc (ch, count + 1);
	int n;
	for (n = 0; n < count; n ++)
		out[n] = ch;

	return out;
}


// Short: Replaces string buffer with string repeated N times
// Notes: None.
char *String_Edit_Repeat (char *s_edit, size_t s_size, const char *s_repeat, int count)
{
	int i;
	s_edit[0] = 0;

	for (i = 0; i < count; i ++)
		strlcat (s_edit, s_repeat, s_size);

	return s_edit;
}

//http://stackoverflow.com/questions/8534274/is-the-strrev-function-not-available-in-linux
char *String_Edit_Reverse (char *s_edit)
{
    int i = strlen (s_edit) - 1, j = 0;

    while (i > j)
		c_swapb (&s_edit[i--], &s_edit[j++]);

	return s_edit;
}


// Short: Deletes count from
// Notes: No safety checks at this time.  Attempts to delete null terminator or exceed buffer are not prohibited.
char *String_Edit_Delete_At (char *s_edit, size_t s_size, size_t offset, size_t num)
{
//	if (offset + num - 1 >= s_size)
//	{
		// 012345
		// frog0   String_Edit_Delete_At(s, 256, 2, 2)
		// XX__0
		char *s_after = &s_edit[offset + num];
		char *s_dest = &s_edit[offset];
		size_t move_num = s_size - offset - num - 1; // +1 for null term, -1 for net length formula
		memmove (s_dest, s_after, move_num); // Take null term with us
		return s_edit;
//	}
//	return NULL;
}

// Short: Replaces all instances of a string within a string or until reaching s_size - 1, returns s_edit
// Notes: No safety checks
char *String_Edit_Range_Delete (char *s_edit, size_t s_size, const char *s_start, const char *s_end)
{
	return String_Edit_Delete_At (s_edit, s_size, s_start - s_edit, s_end - s_start + 1);
}

// Short: Replaces all instances of a string within a string or until reaching s_size - 1
// Notes: None.
char *String_Edit_Replace (char *s_edit, size_t s_size, const char *s_find, const char *s_replace)
{
	char *first_match = s_find && s_find[0] ? strstr(s_edit, s_find) : NULL;

	// Make sure the find string actually exists in the string before going to any effort ...
	if (first_match)
	{
		int s_find_len = strlen (s_find);
		int s_replace_len = strlen(s_replace);
		int delta_len = s_replace_len - s_find_len;

    	// If replacement string length greater than find, duplicate source buffer.
    	// While it is possible to avoid creating this extra buffer
    	// and just memmove the remaining string after each replacement
    	// this would be inefficient.

		char *src_dup = (delta_len > 0) ? strdup (s_edit) : NULL;
		const char *src = src_dup ? src_dup : s_edit;


		// new match is first_match's offset into the source.
		// This may look repetitive but to keep Memory_Constant and regular source same
		char *new_match = (char *)&src[first_match - s_edit];

		char *dst = s_edit;
		const char *buf_start = &dst[0];
		const char *buf_end   = &dst[s_size - 1];
		const char *replace_src;

		while ( new_match )
		{
			// First copy over any characters we skipped
			while (src < new_match)
			{
				if (dst > buf_end)
					goto end_of_buffer;
				*dst++ = *src ++;
			}

			// Now copy over replacement
			for (replace_src = s_replace; *replace_src; )
			{
				if (dst > buf_end)
					goto end_of_buffer;
				*dst++ = *replace_src++;
			}

			// Now advance cursor past find results
			src += s_find_len;
			new_match = strstr(src, s_find);

		} // End of while loop

		// Copy over anything that remains in the buffer
		while (*src)
		{
			if (dst > buf_end)
				goto end_of_buffer;
			*dst++ = *src ++;
		}

end_of_buffer:

		// Null terminate it
		if (dst <= buf_end)
			*dst = 0;
		else if (dst > buf_start)
			dst[-1] = 0; // We would overflow

		if (src_dup)
			free (src_dup);
	}

	return s_edit;
}

char *String_Edit_Replace_Token_Array (char *s_edit, size_t s_size, const char **replace_tokens2)
{
	const char **curt;
	for (curt = replace_tokens2; *curt; curt +=2 ) {
		const char *s_find = curt[0], *s_replace = curt[1];

		String_Edit_Replace (s_edit, s_size, s_find, s_replace); // TODO: # replaces feedback would be nice :(

		//alert ("key:\n\n%s\n\n%s", s_find, s_replace);
	}
	return s_edit;
}




// Short: Replaces all instances of a string within a string or until reaching s_size - 1
// Notes: None.
char *String_Edit_Replace_Memory_Constant (char *s_edit, size_t s_size, const char *s_find, const char *s_replace)
{
	char *first_match = s_find && s_find[0] ? strstr(s_edit, s_find) : NULL;

	// Make sure the find string actually exists in the string before going to any effort ...
	if (first_match)
	{
		int s_find_len = strlen (s_find);
		int s_replace_len = strlen(s_replace);
		int delta_len = s_replace_len - s_find_len;

		const char *src = s_edit;

		// new match is first_match's offset into the source.
		// This may look repetitive but to keep Memory_Constant and regular source same
		char *new_match = (char *)&src[first_match - s_edit];

		char *dst = s_edit;
		const char *buf_start = &dst[0];
		const char *buf_end   = &dst[s_size - 1];

		const char *replace_src;

		while ( new_match )
		{
			// First copy over any characters we skipped
			while (src < new_match)
			{
				if (dst > buf_end)
					goto end_of_buffer;
				*dst++ = *src ++;
			}

			// If replacement string is longer, memmove everything
			// after the current match forward the appropriate amount first ...
			if (delta_len > 0)
			{
				char *move_dst = &new_match[s_replace_len];
				char *move_src = &new_match[s_find_len];
				int move_len = buf_end - move_src - delta_len;
				if (move_len > 0)
					memmove (move_dst, move_src, move_len);

				src += delta_len;
			}

			// Now copy over replacement
			for (replace_src = s_replace; *replace_src; )
			{
				if (dst > buf_end)
					goto end_of_buffer;
				*dst++ = *replace_src++;
			}

			// Now advance cursor past find results
			src += s_find_len;
			new_match = strstr(src, s_find);

		} // End of while loop

		// Copy over anything that remains in the buffer
		while (*src)
		{
			if (dst > buf_end)
				goto end_of_buffer;
			*dst++ = *src ++;
		}

	end_of_buffer:

		// Null terminate it
		if (dst <= buf_end)
			*dst = 0;
		else if (dst > buf_start)
			dst[-1] = 0; // We would overflow

	}

	return s_edit;
}


// Short: Replaces all instances of a character with another character
// Notes: None.
char *String_Edit_Replace_Char (char *s_edit, int ch_find, int ch_replace, reply int *outcount)
{
	char *cursor;
	int n;
	int count;

	for (cursor = s_edit, count = 0, n = 0; *cursor; cursor ++, n++)
	{
		if (*cursor == ch_find)
		{
			int c = *cursor;
			s_edit[n] = ch_replace;
			count ++;
		}
	}

	NOT_MISSING_ASSIGN(outcount, count);

	return s_edit;
}



// Short: Converts all upper case characters to lower case
// Notes: None.
char *String_Edit_To_Lower_Case (char *s_edit)
{
	char *cursor;
	for (cursor = s_edit; *cursor; cursor ++)
	{
		if ( isupper (*cursor) )
			*cursor = tolower (*cursor);
	}
	return s_edit;
}


// Short: Converts first character to upper case, remaining characters to lower case
// Notes: None.
char *String_Edit_To_Proper_Case (char *s_edit)
{
	char *cursor = s_edit;

	// Capitalize the first letter
	if (*cursor && islower(*cursor))
		*cursor = toupper(*cursor);

	cursor ++;

	// Now lower case ensure the rest
	String_Edit_To_Lower_Case (cursor);
	return s_edit;
}


// Short: Converts all lower case characters to upper case
// Notes: None.
char *String_Edit_To_Upper_Case (char *s_edit)
{
	char *cursor;
	for (cursor = s_edit; *cursor; cursor ++)
	{
		if ( islower (*cursor) )
			*cursor = toupper (*cursor);
	}
	return s_edit;
}


// Short: Removes all trailing and leading spaces from a string
// Notes: None.
char *String_Edit_Trim (char *s_edit)
{
	String_Edit_RTrim_Whitespace_Including_Spaces (s_edit);
	String_Edit_LTrim_Whitespace_Including_Spaces (s_edit);
	return s_edit;
}


// Short: Dequotes a string, if it is quoted.  Otherwise does nothing.
// Notes: None.
char *String_Edit_Unquote (char *s_edit)
{
	size_t len = strlen(s_edit);
	if (len > 1 && s_edit[0] == '\"' && s_edit[len - 1] == '\"')
	{
		s_edit[len - 1] = 0;
		memmove (s_edit, &s_edit[1], len - 2 + 1); // size is reduced by 2, add +1 for null term for memmove op
	}
	return s_edit;
}


// Short: SPACE_CHAR_32 to 126 stays.  Everything else except \n and \r g
// Notes: None.
char *String_Edit_For_Stdout_Ascii (char *s_edit)
{
	unsigned char *cursor;
	int c;
	cbool should_print;

	for (cursor = s_edit; *cursor; cursor ++)
	{
		c = *cursor;
		should_print = in_range (SPACE_CHAR_32, c, 126) || c == '\n'; // // && /* c != '\r' && */  c != '\t')

		if (!should_print)
			*cursor = '#';
	}

	return s_edit;
}



// Short: Replaces all white-space characters (char < SPACE_CHAR_32) with spaces (char SPACE_CHAR_32)
// Notes: None.
char *String_Edit_Whitespace_To_Space (char *s_edit)
{
	char *cursor;
	int c;

	for (cursor = s_edit; *cursor; cursor ++)
	{
		c = *cursor;
		if (c < SPACE_CHAR_32)
			*cursor = SPACE_CHAR_32;
	}

	return s_edit;
}


// Short: Returns pointer to occurence of string --- considering case --- or NULL if not found
// Notes: Offers no advantage over strstr.
char *String_Find (const char *s, const char *s_find)
{
	char *search = strstr (s, s_find);
	return search;
}


// Short: Returns pointer to occurence of string --- ignoring case --- or NULL if not found
// Notes: Offers no advantage over strcasestr.
char *String_Find_Caseless (const char *s, const char *s_find)
{
	char *search = strcasestr (s, s_find);
	return search;
}


// Short: Returns pointer to first character after instance of find string --- considering case, NULL if not found
// Notes: None.
char *String_Find_Skip_Past (const char *s, const char *s_find)
{
	char *search = strstr (s, s_find);
	if (search)
	{
		size_t len = strlen(s_find);
		return &search[len];
	}
	return NULL;
}


// Short: Returns pointer to beginning of instance of find string --- considering case, NULL if not found
// Notes: Offers no advantage over strrstr.
char *String_Find_Reverse (const char *s, const char *s_find)
{
	char *search = strrstr (s, s_find);
	return search;
}


// Short: Returns pointer to first instance of character, NULL if not found
// Notes: Offers no advantage over strchr.
char *String_Find_Char (const char *s, int ch_findchar)
{
	const char *s_end = String_Find_End (s);

	if (s_end)
		return String_Range_Find_Char (s, s_end, ch_findchar);

	return NULL;
}


// Short: Returns pointer to Nth occurrence of character, NULL if no such instance
// Notes: None.
char *String_Find_Char_Nth_Instance (const char *s, int ch_findchar, int n_instance)
{
	const char *search;
	int count;

	for (search = s, count = 0; *search; search ++)
	{
		if (*search == ch_findchar && ++count == n_instance)
			return (char *)search;
	}

	return NULL;
}


// Short: Returns pointer to last instance of character, NULL if not found
// Notes: Offers no advantage over strrchr.
char *String_Find_Char_Reverse (const char *s, int ch_findchar)
{
	return (char *) strrchr(s, ch_findchar);
}


// Short: Returns pointer to last character of string or NULL if zero length string.
// Notes: None.
char *String_Find_End (const char *s)
{
	size_t len = strlen(s);
	const char *s_end = len > 0 ?  &s[len-1] : NULL; // 4 length = 0 through 3 = 4 - 1 = s

	return (char *)s_end;
}


// Short: Determines if Microsoft Excel would call the string a number.  Rather accurate.
// Note: Works for typical usage for simple strings meant for CSV export or copy/paste.

int String_Does_Have_Excel_Numeric_Representation (const char *s)
{
//	const char *numbers[] = { "5", "0", ".5", "5.", "\"5", "  5"," 5 ", NULL };
//	const char *not_numbers[] = { "x5", "5x", ".", "5..", "..5", "5\"","  5  6 ", NULL };

	char *sc_a = strdup (s), ch;
	int result, pos, anydigit, dots;

	// Leading and trailing spaces are ignored.
	String_Edit_Trim (sc_a); // memmoves

	for (result = 1, pos = dots = anydigit = 0; (ch = sc_a[pos]); pos++)
	{
		if (isdigit (ch))
		{
			if (!anydigit) anydigit = 1;
		}
		else if (ch == '.')
		{ // a period
			dots ++;
			if (dots > 1)
			{
				result = 0; // more than 1 dot is never a number
				break;
			}
		}
		else if (ch == '\"')
		{ // a quote
			if (pos == 0) // Ignore a leading quote
				continue;
			result = 0; // If quote isn't leading character
			break;
		}
		else
		{ // Non-digit
			result = 0; // Non digit and non-period is never a number
			break;
		}
	}

	free (sc_a);
	if (result && anydigit) return 1;
	return 0;
}


// Short: Counts the number of instances of a character in a sub-string from start through end, 0 if not found
// Notes: None.
int String_Range_Count_Char (const char *s_start, const char *s_end, int ch_findchar)
{
	int count;
	const char *search;
	for (search = s_start, count = 0; *search && search <= s_end; search ++)
		if (*search == ch_findchar)
			count ++;
	return count;
}


// Short: Finds a character in a sub-string, returns NULL if not found
// Notes: Advantage: strchr could waste time searching beyond end of scope desired (beyond end of range)
char *String_Range_Find_Char (const char *s_start, const char *s_end, int ch_findchar)
{
	const char *search;
	for (search = s_start; *search && search <= s_end; search ++)
	{
		if (*search == ch_findchar)
			return (char *)search;
	}
	return NULL;
}


// Short: Copes a sub-string into a string ensuring null termination.
// Notes: If buffer size is 0, no operation occurs.
char *String_Range_Copy (char *s_dest, size_t siz, const char *src_start, const char *src_end)
{
	char *dst;
	const char *src;
	size_t copied;

	if (siz > 0)
	{
		for (dst = s_dest, src = src_start, copied = 0; *src && src <= src_end; )
		{
			if (copied >= siz)
				break;

			*dst++ = *src++;
			copied ++;
		}

		*dst = 0; // NULL at end.
	}
	return s_dest;
}

// Short: Copes a sub-string into a string ensuring null termination.
// Notes: If buffer size is 0, no operation occurs.
char *String_Length_Copy (char *s_dest, size_t siz, const char *src_start, size_t length)
{
	if (length > siz)
		length = siz;

	if (length > 0)
		String_Range_Copy (s_dest, siz, src_start, &src_start[length - 1]);

	return s_dest;
}

// Short: Finds position of string.
// Notes: Returns -1 on not found
int String_Pos_Find (const char *s, const char *s_find)
{
	const char *sfound = strstr (s, s_find);
	if (sfound)
		return sfound - s;
	return -1;
}

// Short: Finds position of string.
// Notes: Returns -1 on not found
int String_Pos_Find_Char (const char *s, int ch_findchar)
{
	const char *sfound = String_Find_Char (s, ch_findchar);
	if (sfound)
		return sfound - s;
	return -1;
}

// Short: Missing
// Notes: Advantage: strrchr could waste time searching beyond end of scope desired (beyond end of range)
char *String_Range_Find_Char_Reverse (const char *s_start, const char *s_end, int ch_findchar)
{
	const char *search;
	for (search = s_end; *search && search >= s_start; search --)
	{
		if (*search == ch_findchar)
			return (char *)search;
	}
	return NULL;
}


// Short: Returns character past first instance of char to find, or returns string if none found
// Notes: Offers no advantage over strchr, but functions differently as strchr returns NULL if nothing found
char *String_Skip_Char (const char *s, int ch_findchar)
{
	const char *search = strchr (s, ch_findchar);
	return (search ? (char*)&search[1] : (char*)s);
}

char *String_Skip_Char_Reverse (const char *s, int ch_findchar)
{
	const char *search = strrchr (s, ch_findchar);

	return (search ? (char *)&search[1] : (char *)s);
}



// Short: Returns a pointer beyond any leading white-space (char < SPACE_CHAR_32) in a string except for spaces (char SPACE_CHAR_32)
// Notes: None.
char *String_Skip_WhiteSpace_Excluding_Space (const char *s)
{
	for ( ; *s && *s < ' '; s++); // <---- Notice the semicolon

	return (char *)s; // New cursor spot
}


// Short: Returns a pointer beyond any leading white-space (char <= SPACE_CHAR_32) in a string including for spaces (char SPACE_CHAR_32)
// Notes: None.
char *String_Skip_WhiteSpace_Including_Space (const char *s)
{
	for ( ; *s && *s <= ' '; s++); // <---- Notice the semicolon

	return (char *)s; // New cursor spot
}

char *String_Skip_NonWhiteSpace (const char *s)
{
	for ( ; *s && *s > ' '; s++); // <---- Notice the semicolon

	return (char *)s; // New cursor spot
}

static int sString_To_Array_Index_Fn (const char* string, const char* string_array[], int caseless_compare)
{
	int (*compare_fn) (const char *, const char *);
	const char* cursor;
	int i;

	compare_fn = caseless_compare ? strcasecmp : strcmp;
	for (i = 0; string_array[i]; i ++)
	{
		cursor = string_array[i];
		if (!compare_fn (string, cursor))
			return i;
	}

	return i;
}


// Short: Returns index in array const like char *list[] = { a, b, c, d, NULL};
// Notes: Array must be NULL terminated.
int String_To_Array_Index (const char* s, const char* s_array[])
{
	return sString_To_Array_Index_Fn (s, s_array, 0 /* false: case matters */ );
}


// Short: Returns index in array like const char *list[] = { a, b, c, d, NULL};
// Notes: Array must be NULL terminated.
int String_To_Array_Index_Caseless (const char* s, const char* s_array[])
{
	return sString_To_Array_Index_Fn (s, s_array, 1 /* true: caseless */ );
}

// Short: Returns 1 if found an instance of find in string using a delimiter i.e. ("frogs,dogs,apple", "apple", ',')
// Notes: Finds match to whole word within a string (with specified delimiter and null count as delimiters)
int String_List_Match (const char *s, const char *s_find, int ch_delim)
{
	const char	*start, *matchspot, *nextspot;
	int			itemlen;

	// Check for disqualifying situations ...
	if (s == NULL || s_find == NULL || *s_find == 0 || strchr (s_find, ch_delim))
		return 0; // Failed

	itemlen = (int)strlen (s_find);

	for (start = s ; (matchspot = strstr (start, s_find) ) ; start = nextspot)
	{
		nextspot = matchspot + itemlen;

		// Validate that either the match begins the string or the previous character is a comma
		// And that the terminator is either null or a comma.  This protects against partial
		// matches

		if ((matchspot == start || *(matchspot - 1) == ch_delim) && (*nextspot == 0 || *nextspot == ch_delim))
			return 1; // Match matchspot;
	}

	return 0; // No matches
}






// Short: sprintf to temp string (size of static temp string buffer is 1024) which does not need to be freed
// Notes: There are 32 static temp buffers which are cycled.  Best for short-lived vars in non-recursive functions.
char *va (const char *format, ...)
{
	static char 	buffers[CORE_STRINGS_VA_ROTATING_BUFFERS_COUNT_32][CORE_STRINGS_VA_ROTATING_BUFFER_BUFSIZ_1024];
	static size_t 	sizeof_a_buffer 	= sizeof(buffers[0]);
	static size_t 	num_buffers			= sizeof(buffers) / sizeof(buffers[0]);
	static size_t 	cycle = 0;

	char			*buffer_to_use = buffers[cycle];
	va_list 		args;

	va_start 		(args, format);
	c_vsnprintf 	(buffer_to_use, sizeof_a_buffer, format, args);
	va_end 			(args);

	// Cycle through to next buffer for next time function is called

	if (++cycle >= num_buffers)
		cycle = 0;

	return buffer_to_use;
}




#ifdef CORE_NEEDS_STRLCPY_STRLCAT

//========================================================
// strlcat and strlcpy, from OpenBSD

/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

size_t strlcat(char *dst, const char *src, size_t siz)
{
	register char *d = dst;
	register const char *s = src;
	register size_t n = siz;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0')
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return(dlen + strlen(s));
	while (*s != '\0')
	{
		if (n != 1)
		{
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return(dlen + (s - src));	/* count does not include NUL */
}

size_t strlcpy(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0 && --n != 0)
	{
		do
		{
			if ((*d++ = *s++) == 0)
				break;
		} while (--n != 0);
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0)
	{
		if (siz != 0)
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}

	return(s - src - 1);	/* count does not include NUL */
}

#endif // CORE_NEEDS_STRLCPY_STRLCAT

#ifdef CORE_NEEDS_STRCASESTR
/*
FUNCTION
	<<strcasestr>>---case-insensitive character string search

INDEX
	strcasestr

ANSI_SYNOPSIS
	#include <string.h>
	char *strcasestr(const char *<[s]>, const char *<[find]>);

TRAD_SYNOPSIS
	#include <string.h>
	int strcasecmp(<[s]>, <[find]>)
	char *<[s]>;
	char *<[find]>;

DESCRIPTION
	<<strcasestr>> searchs the string <[s]> for
	the first occurrence of the sequence <[find]>.  <<strcasestr>>
	is identical to <<strstr>> except the search is
	case-insensitive.

RETURNS

	A pointer to the first case-insensitive occurrence of the sequence
	<[find]> or <<NULL>> if no match was found.

PORTABILITY
<<strcasestr>> is in the Berkeley Software Distribution.

<<strcasestr>> requires no supporting OS subroutines. It uses
tolower() from elsewhere in this library.

QUICKREF
	strcasestr
*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * The quadratic code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/* Linear algorithm Copyright (C) 2008 Eric Blake
 * Permission to use, copy, modify, and distribute the linear portion of
 * software is freely granted, provided that this notice is preserved.
 */

#include <ctype.h>
#include <string.h>

/*
 * Find the first occurrence of find in s, ignore case.
 */
char * strcasestr(const char *s, const char *find)
{

  /* Less code size, but quadratic performance in the worst case.  */
	char c, sc;
	size_t len;

	if ((c = *find++) != 0) {
		c = tolower((unsigned char)c);
		len = strlen(find);
		do {
			do {
				if ((sc = *s++) == 0)
					return (NULL);
			} while ((char)tolower((unsigned char)sc) != c);
		} while (strncasecmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);

}

#endif // CORE_NEEDS_STRCASESTR

#ifdef CORE_NEEDS_STRRSTR

/* Strrstr.c, included for those computers that do not have it. */
/* Written by Kent Irwin, irwin@leland.stanford.edu.  I am
   responsible for bugs */

char *strrstr(const char *s1, const char *s2)
{
	const char *sc2, *psc1, *ps1;

	if (*s2 == '\0')
		return((char *)s1);

	ps1 = s1 + strlen(s1);

	while(ps1 != s1) {
		--ps1;
		for (psc1 = ps1, sc2 = s2; ; )
			if (*(psc1++) != *(sc2++))
				break;
			else if (*sc2 == '\0')
				return ((char *)ps1);
	}
	return ((char *)NULL);
}

#endif // CORE_NEEDS_STRRSTR

#ifdef CORE_NEEDS_STRNLEN
size_t strnlen (const char *string, size_t maxlen)
{
  const char *end = memchr (string, '\0', maxlen);
  return end ? (size_t)(end - string) : maxlen;  // end - string must always be >=0 and function returns size_t
}
#endif // CORE_NEEDS_STRNLEN

#ifdef CORE_NEEDS_STRNDUP
char * strndup (const char *s, size_t n)
{
  size_t len = strnlen (s, n);
  char *new = malloc (len + 1);

  if (new == NULL)
    return NULL;

  new[len] = '\0';
  return memcpy (new, s, len);
}
#endif // CORE_NEEDS_STRNDUP


#ifdef CORE_NEEDS_STRPTIME
/*
 * Ported from NetBSD to Windows by Ron Koenderink, 2007
 */

/*      $NetBSD: strptime.c,v 1.25 2005/11/29 03:12:00 christos Exp $   */

/*-
 * Copyright (c) 1997, 1998, 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was contributed to The NetBSD Foundation by Klaus Klein.
 * Heavily optimised by David Laight
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#if !defined(_WIN32)
#include <sys/cdefs.h>
#endif

#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: strptime.c,v 1.25 2005/11/29 03:12:00 christos Exp $");
#endif

#if !defined(_WIN32)
#include "namespace.h"
#include <sys/localedef.h>
#else
typedef unsigned char u_char;
typedef unsigned int uint;
#endif
#include <ctype.h>
#include <locale.h>
#include <string.h>
#include <time.h>
#if !defined(_WIN32)
#include <tzfile.h>
#endif

#ifdef __weak_alias
__weak_alias(strptime,_strptime)
#endif

#if !defined(_WIN32)
#define _ctloc(x)               (_CurrentTimeLocale->x)
#else
#define _ctloc(x)   (x)
const char *abday[] = {
        "Sun", "Mon", "Tue", "Wed",
        "Thu", "Fri", "Sat"
};
const char *day[] = {
        "Sunday", "Monday", "Tuesday", "Wednesday",
        "Thursday", "Friday", "Saturday"
};
const char *abmon[] =   {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
const char *mon[] = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
};
const char *am_pm[] = {
        "AM", "PM"
};
char *d_t_fmt = "%a %Ef %T %Y";
char *t_fmt_ampm = "%I:%M:%S %p";
char *t_fmt = "%H:%M:%S";
char *d_fmt = "%m/%d/%y";
#define TM_YEAR_BASE 1900
#define __UNCONST(x) ((void *)(((const char *)(x) - (const char *)0) + (char *)0))

#endif
/*
 * We do not implement alternate representations. However, we always
 * check whether a given modifier is allowed for a certain conversion.
 */
#define ALT_E                   0x01
#define ALT_O                   0x02
#define LEGAL_ALT(x)            { if (alt_format & ~(x)) return NULL; }


static const u_char *conv_num(const unsigned char *, int *, uint, uint);
static const u_char *find_string(const u_char *, int *, const char * const *,
        const char * const *, int);


char *
strptime(const char *buf, const char *fmt, struct tm *tm)
{
        unsigned char c;
        const unsigned char *bp;
        int alt_format, i, split_year = 0;
        const char *new_fmt;

        bp = (const u_char *)buf;

        while (bp != NULL && (c = *fmt++) != '\0') {
                /* Clear `alternate' modifier prior to new conversion. */
                alt_format = 0;
                i = 0;

                /* Eat up white-space. */
                if (isspace(c)) {
                        while (isspace(*bp))
                                bp++;
                        continue;
                }

                if (c != '%')
                        goto literal;


again:          switch (c = *fmt++) {
                case '%':       /* "%%" is converted to "%". */
literal:
                        if (c != *bp++)
                                return NULL;
                        LEGAL_ALT(0);
                        continue;

                /*
                 * "Alternative" modifiers. Just set the appropriate flag
                 * and start over again.
                 */
                case 'E':       /* "%E?" alternative conversion modifier. */
                        LEGAL_ALT(0);
                        alt_format |= ALT_E;
                        goto again;

                case 'O':       /* "%O?" alternative conversion modifier. */
                        LEGAL_ALT(0);
                        alt_format |= ALT_O;
                        goto again;

                /*
                 * "Complex" conversion rules, implemented through recursion.
                 */
                case 'c':       /* Date and time, using the locale's format. */
                        new_fmt = _ctloc(d_t_fmt);
                        goto recurse;

                case 'D':       /* The date as "%m/%d/%y". */
                        new_fmt = "%m/%d/%y";
                        LEGAL_ALT(0);
                        goto recurse;

                case 'R':       /* The time as "%H:%M". */
                        new_fmt = "%H:%M";
                        LEGAL_ALT(0);
                        goto recurse;

                case 'r':       /* The time in 12-hour clock representation. */
                        new_fmt =_ctloc(t_fmt_ampm);
                        LEGAL_ALT(0);
                        goto recurse;

                case 'T':       /* The time as "%H:%M:%S". */
                        new_fmt = "%H:%M:%S";
                        LEGAL_ALT(0);
                        goto recurse;

                case 'X':       /* The time, using the locale's format. */
                        new_fmt =_ctloc(t_fmt);
                        goto recurse;

                case 'x':       /* The date, using the locale's format. */
                        new_fmt =_ctloc(d_fmt);
                    recurse:
                        bp = (const u_char *)strptime((const char *)bp,
                                                            new_fmt, tm);
                        LEGAL_ALT(ALT_E);
                        continue;

                /*
                 * "Elementary" conversion rules.
                 */
                case 'A':       /* The day of week, using the locale's form. */
                case 'a':
                        bp = find_string(bp, &tm->tm_wday, _ctloc(day),
                                        _ctloc(abday), 7);
                        LEGAL_ALT(0);
                        continue;

                case 'B':       /* The month, using the locale's form. */
                case 'b':
                case 'h':
                        bp = find_string(bp, &tm->tm_mon, _ctloc(mon),
                                        _ctloc(abmon), 12);
                        LEGAL_ALT(0);
                        continue;

                case 'C':       /* The century number. */
                        i = 20;
                        bp = conv_num(bp, &i, 0, 99);

                        i = i * 100 - TM_YEAR_BASE;
                        if (split_year)
                                i += tm->tm_year % 100;
                        split_year = 1;
                        tm->tm_year = i;
                        LEGAL_ALT(ALT_E);
                        continue;

                case 'd':       /* The day of month. */
                case 'e':
                        bp = conv_num(bp, &tm->tm_mday, 1, 31);
                        LEGAL_ALT(ALT_O);
                        continue;

                case 'k':       /* The hour (24-hour clock representation). */
                        LEGAL_ALT(0);
                        /* FALLTHROUGH */
                case 'H':
                        bp = conv_num(bp, &tm->tm_hour, 0, 23);
                        LEGAL_ALT(ALT_O);
                        continue;

                case 'l':       /* The hour (12-hour clock representation). */
                        LEGAL_ALT(0);
                        /* FALLTHROUGH */
                case 'I':
                        bp = conv_num(bp, &tm->tm_hour, 1, 12);
                        if (tm->tm_hour == 12)
                                tm->tm_hour = 0;
                        LEGAL_ALT(ALT_O);
                        continue;

                case 'j':       /* The day of year. */
                        i = 1;
                        bp = conv_num(bp, &i, 1, 366);
                        tm->tm_yday = i - 1;
                        LEGAL_ALT(0);
                        continue;

                case 'M':       /* The minute. */
                        bp = conv_num(bp, &tm->tm_min, 0, 59);
                        LEGAL_ALT(ALT_O);
                        continue;

                case 'm':       /* The month. */
                        i = 1;
                        bp = conv_num(bp, &i, 1, 12);
                        tm->tm_mon = i - 1;
                        LEGAL_ALT(ALT_O);
                        continue;

                case 'p':       /* The locale's equivalent of AM/PM. */
                        bp = find_string(bp, &i, _ctloc(am_pm), NULL, 2);
                        if (tm->tm_hour > 11)
                                return NULL;
                        tm->tm_hour += i * 12;
                        LEGAL_ALT(0);
                        continue;

                case 'S':       /* The seconds. */
                        bp = conv_num(bp, &tm->tm_sec, 0, 61);
                        LEGAL_ALT(ALT_O);
                        continue;

                case 'U':       /* The week of year, beginning on sunday. */
                case 'W':       /* The week of year, beginning on monday. */
                        /*
                         * XXX This is bogus, as we can not assume any valid
                         * information present in the tm structure at this
                         * point to calculate a real value, so just check the
                         * range for now.
                         */
                         bp = conv_num(bp, &i, 0, 53);
                         LEGAL_ALT(ALT_O);
                         continue;

                case 'w':       /* The day of week, beginning on sunday. */
                        bp = conv_num(bp, &tm->tm_wday, 0, 6);
                        LEGAL_ALT(ALT_O);
                        continue;

                case 'Y':       /* The year. */
                        i = TM_YEAR_BASE;       /* just for data sanity... */
                        bp = conv_num(bp, &i, 0, 9999);
                        tm->tm_year = i - TM_YEAR_BASE;
                        LEGAL_ALT(ALT_E);
                        continue;

                case 'y':       /* The year within 100 years of the epoch. */
                        /* LEGAL_ALT(ALT_E | ALT_O); */
                        bp = conv_num(bp, &i, 0, 99);

                        if (split_year)
                                /* preserve century */
                                i += (tm->tm_year / 100) * 100;
                        else {
                                split_year = 1;
                                if (i <= 68)
                                        i = i + 2000 - TM_YEAR_BASE;
                                else
                                        i = i + 1900 - TM_YEAR_BASE;
                        }
                        tm->tm_year = i;
                        continue;

                /*
                 * Miscellaneous conversions.
                 */
                case 'n':       /* Any kind of white-space. */
                case 't':
                        while (isspace(*bp))
                                bp++;
                        LEGAL_ALT(0);
                        continue;


                default:        /* Unknown/unsupported conversion. */
                        return NULL;
                }
        }

        return __UNCONST(bp);
}


static const u_char *
conv_num(const unsigned char *buf, int *dest, uint llim, uint ulim)
{
        uint result = 0;
        unsigned char ch;

        /* The limit also determines the number of valid digits. */
        uint rulim = ulim;

        ch = *buf;
        if (ch < '0' || ch > '9')
                return NULL;

        do {
                result *= 10;
                result += ch - '0';
                rulim /= 10;
                ch = *++buf;
        } while ((result * 10 <= ulim) && rulim && ch >= '0' && ch <= '9');

        if (result < llim || result > ulim)
                return NULL;

        *dest = result;
        return buf;
}

static const u_char *
find_string(const u_char *bp, int *tgt, const char * const *n1,
                const char * const *n2, int c)
{
        int i;
        unsigned int len;

        /* check full name - then abbreviated ones */
        for (; n1 != NULL; n1 = n2, n2 = NULL) {
                for (i = 0; i < c; i++, n1++) {
                        len = strlen(*n1);
                        if (strncasecmp(*n1, (const char *)bp, len) == 0) {
                                *tgt = i;
                                return bp + len;
                        }
                }
        }

        /* Nothing matched */
        return NULL;
}
#endif // CORE_NEEDS_STRPTIME


#ifdef CORE_NEEDS_VSNPRINTF_SNPRINTF

	// On other platforms, vsnprintf can do what c_vsnprintf_alloc is needed for by doing it twice.

	#ifdef PLATFORM_WINDOWS
		// The performance decreases once really large sizes are hit.  But how often are we going to be dealing with super-massive strings.
		char *_length_vsnprintf (cbool just_test, reply int *created_length, reply size_t *created_bufsize, const char *fmt, va_list args)
		{
			size_t bufsize = 8; // Was 2.  Set to 8 which is a defacto 16.
			char *buffer = NULL;
			int length = -1;

			// We need a length of 1 greater
			while (length == -1 || (size_t)length >= bufsize)
			{
				bufsize = bufsize * 2;
				buffer = realloc (buffer, bufsize);
		//		logd ("c_vsnprintf_alloc: buffer size %d", bufsize);

				// For _vsnprintf, if the number of bytes to write exceeds buffer, then count bytes are written
				// and ñ1 is returned.

				length = _vsnprintf (buffer, bufsize, fmt, args);
			}

			// Reduce the allocation to a 16 padded size, which has nothing to do with alignment but rather our stringa spec.
			// To decrease reallocation for small string changes but using a blocksize.  Leave alignment to calloc/malloc.

			if (just_test)
				free  (buffer);
			else
			{
				bufsize = roundup_16 (length + 1);
				buffer = realloc (buffer, bufsize); // Reduce the allocation.
			}

		//	logd ("c_vsnprintf_alloc: '%s'" NEWLINE "Length is %d", buffer, length);
			NOT_MISSING_ASSIGN(created_length, length);
			NOT_MISSING_ASSIGN(created_bufsize, bufsize);
			return buffer;
		}


	#else // Non-Windows

		// The performance decreases once really large sizes are hit.  But how often are we going to be dealing with super-massive strings.
		char *_length_vsnprintf (cbool just_test, reply int *created_length, reply size_t *created_bufsize, const char *fmt, va_list args)
		{
			va_list args_copy;
			va_copy (args_copy, args);
			int length = vsnprintf(NULL, 0, fmt, args_copy);
			va_end (args_copy);
			size_t bufsize = roundup_16 (length + 1);
			char *buffer = NULL;

			if (!just_test) {
				va_list args_copy;
				buffer = calloc(bufsize,1);
				va_copy (args_copy, args);
				int length2 = vsnprintf (buffer, length + 1, fmt, args_copy); // I'm not sure this null terminates?
				va_end (args_copy);

				//buffer[length] = 0;
			}

			//	logd ("c_vsnprintf_alloc: '%s'" NEWLINE "Length is %d", buffer, length);
			NOT_MISSING_ASSIGN(created_length, length);
			NOT_MISSING_ASSIGN(created_bufsize, bufsize);
			return buffer;
		}


	#endif // !PLATFORM_WINDOWS
#endif // CORE_NEEDS_VSNPRINTF_SNPRINTF


char *c_strdupf (const char *fmt, ...)
{
	VA_EXPAND_ALLOC (text, length, bufsiz, fmt);

//	logd ("Allocated " QUOTED_S " with bufsize %d", newstring, bufsiz);

	return text; // VA_EXPAND_ALLOC free not necessary as this is alloc function
}


char *c_len_strdupf (reply int *len, const char *fmt, ...) // Same except first param returns length
{
	VA_EXPAND_ALLOC (text, length, bufsiz, fmt);

//	logd ("Allocated " QUOTED_S " with bufsize %d", newstring, bufsiz);
	NOT_MISSING_ASSIGN (len, length);

	return text; // VA_EXPAND_ALLOC free not necessary as this is alloc function
}


char *String_Range_Dup_Alloc(const char *s_start, const char *s_end)
{
	int len = s_end - s_start + 1;

	return (len > 0) ? strndup (s_start, len) : NULL;
}


char *String_Replace_Alloc (const char *s, const char *s_find, const char *s_replace)
{
	// Some day, preprocessor macros might be as reliable.  Or perhaps someday, headers will auto-generate and all functions should be explicitly defined.
	return String_Replace_Len_Count_Alloc (s, s_find, s_replace, NULL, NULL, NULL);
}

char *String_Replace_Len_Count_Alloc (const char *s, const char *s_find, const char *s_replace, reply int *created_length, reply size_t *created_bufsize, reply int *replace_count)
{
	char *newstring_o;

	// CASE MATTERS!
	// Determine the number of instances of s_find
	int s_len = strlen(s), find_len = strlen(s_find), replace_len = strlen(s_replace);
	int delta_instance_len = replace_len - find_len; // How much we increase per instance
	// Number of instances?
	// strcasestr
	int count = 0;
	const char *cursor;


	if (!find_len) {
//		log_warn ("Passed empty replacement string");

		NOT_MISSING_ASSIGN(replace_count, count);
		NOT_MISSING_ASSIGN(created_length, strlen (s));
		NOT_MISSING_ASSIGN(created_bufsize, strlen (s) + 1);

		return strdup(s);
	}

	for (cursor = s, count = 0; (cursor = strstr(cursor, s_find));  count ++)
		cursor += find_len; // Advance cursor

	block_start__

	int delta_len = delta_instance_len * count;
	size_t bufsiz = s_len + delta_len + 1; // delta_len might be positive or negative
	size_t minbufsize = s_len + 1;
	size_t tempbufsize = bufsiz > minbufsize ? bufsiz : minbufsize; // Temp bufsize is greater of the 2

	newstring_o = malloc (tempbufsize);
	strlcpy (newstring_o, s, tempbufsize);

	// This is gonna get tested!
	String_Edit_Replace (newstring_o, tempbufsize, s_find, s_replace);

	if (tempbufsize > bufsiz) // Over allocation can now be corrected.
		newstring_o = realloc (newstring_o, bufsiz); // Shorten.  Or should we really bother?  Yes, let's bother.  What if source was a monster.

	NOT_MISSING_ASSIGN(replace_count, count);
	NOT_MISSING_ASSIGN(created_length, bufsiz - 1);
	NOT_MISSING_ASSIGN(created_bufsize, bufsiz);

	__block_end

	return newstring_o;
}


// Our version doesn't comply with C and instead returns size if buffer would have overflowed.
int c_vsnprintf (char * s, size_t n, const char *fmt, va_list arg)
{
	int ret;

	// For _vsnprintf, if the number of bytes to write exceeds
	// buffer, then count bytes are written and n is returned.

#ifdef PLATFORM_WINDOWS
	#ifdef __VISUAL_STUDIO_6__
		ret = _vsnprintf (s, n, fmt, arg);
	#else
		ret = _vsnprintf (s, n, fmt, arg); // To do ...
	#endif
#else // ! PLATFORM_WINDOWS ...
	ret = vsnprintf (s, n, fmt, arg);
#endif // ! PLATFORM_WINDOWS

	// Conditionally null terminate the string
	if (ret < 0)
		ret = (int)n;
	if (n == 0)	/* no buffer */
		return ret;
	if ((size_t)ret >= n)
		s[n - 1] = '\0';

	return ret;

}


int c_snprintfc (char * s, size_t n, const char *fmt, ...)
{
	va_list args; // pointer to the list of arguments
	int result;

		va_start (args, fmt);
		result = c_vsnprintf (s, n, fmt, args);
	va_end (args);

	return result;
}

// #endif // CORE_NEEDS_VSNPRINTF_SNPRINTF  Was here.  Didn't seem right.


// Short: wildcard compare (i.e. if ( wildcmp ("bl?h.*", "blah.jpg") )
// Notes: http://www.codeproject.com/Articles/1088/Wildcard-string-compare-globbing
// true/false

static int _donothing(int a)
{
	return a;
}

static cbool wildcmpx (const char *s, const char *wild_pattern, cbool case_matters)
{
	// Written by Jack Handy - <A href="mailto:jakkhandy@hotmail.com">jakkhandy@hotmail.com</A>
	const char *cp = NULL, *mp = NULL;
	typedef int (*charcase_fn_t) (int);
	charcase_fn_t case_fn = case_matters ? _donothing : tolower;

	// Comparing leading until we hit an asterisk
	while (*s && *wild_pattern != '*') {
		if ( case_fn(*wild_pattern) != case_fn(*s) && *wild_pattern != '?')
			return false;

		wild_pattern++, s++;
	}

	while (*s) {
		if (*wild_pattern == '*') {
			if (!*++wild_pattern)
				return true; // Hit asterisk terminator in wild_pattern s

			mp = wild_pattern, cp = s + 1;
		}
		else if (case_fn(*wild_pattern) == case_fn(*s) || *wild_pattern == '?')
			wild_pattern++, s++;
		else
			wild_pattern = mp, s = cp++;
	}

	// Trailing asterisk
	while (*wild_pattern == '*')
		wild_pattern++;

	return !*wild_pattern;
}



//String_Test_Reject (test, "<valid:alpha,numeric,space>");
// <validchars:alpha,numeric,space>
// <notempty>, <notrim>
const char *String_Test_Reject (const char *s_test, const char *textflags)
{
	const char *fail_reason = NULL;

#define FAILOUT(msg) { fail_reason = msg; goto cleanup; }
#define CLEANUP freenull (command_string); freenull (command_prefix);  freenull (flags_string);

	char *command_string = NULL, *command_prefix = NULL, *flags_string = NULL;

	const char *cursor, *s_colon, *s_less, *ch;
	int s_test_len;

	if (!textflags) return NULL; // Didn't fail, there are just no test criteria
	if (!s_test) FAILOUT ("Input cannot be null")

	s_test_len = strlen(s_test);

	cursor = textflags;
	while (*cursor) {
		CLEANUP // Don't want accumulations while running through loop

		if (*cursor != '<') FAILOUT ("Invalid criteria didn't encounter <")

		s_less = strstr(&cursor[1], ">");
		if (!s_less) FAILOUT( "Invalid criteria didn't encounter >")

		command_string = String_Range_Dup_Alloc (cursor, s_less);

		cursor = &s_less[1]; // Move cursor beyond ">" for next iteration when/if that happens

		//
		// Check command_string - standalones
		//

		if (!strcasecmp (command_string, "<notempty>")) {
			if (!s_test[0]) FAILOUT	("Input cannot be empty")

			continue;
		}

		if (!strcasecmp (command_string, "<notrim>")) {
			if (s_test_len)
				if (iswhitespace(s_test[0]) || iswhitespace (s_test[s_test_len - 1]))
					FAILOUT ("Input cannot have leading or trailing whitespace")

			continue;
		}

		//
		// Check command_string - compounders
		//

		s_colon = strstr(command_string, ":");
		if (!s_colon) FAILOUT ("Couldn't colon")

		command_prefix = String_Range_Dup_Alloc (command_string, s_colon);
		flags_string = strdup (s_colon);	flags_string[0] = ','; flags_string[strlen(flags_string) - 1] = ',';

		if (!strcasecmp (command_prefix, "<validchars:")) {
			int pass_alpha		= !!strstr (flags_string, ",alpha,");
			int pass_numeric	= !!strstr (flags_string, ",numeric,");
			int pass_space		= !!strstr (flags_string, ",spaces,");

			const char *validchars_reason = va("Invalid characters: Text may only contain:%s%s%s",
				pass_alpha ? " letters" : "",
				pass_numeric ? " numbers" : "",
				pass_space ? " spaces" : "");

			flags_string = core_free (flags_string);

			for (ch = s_test; *ch; ch ++) {
				if (pass_alpha   && isalpha (*ch)) continue;
				if (pass_numeric && isdigit (*ch)) continue;
				if (pass_space   && *ch == ' '   ) continue;

				FAILOUT ( validchars_reason )
			}

			continue;
		}

		FAILOUT ( va("Unrecognized command " QUOTED_S, command_prefix) )

	}

cleanup:

	CLEANUP
#undef CLEANUP
#undef FAILOUT
	return fail_reason;
}


// Short: multi-wildcard compare (wildcard compare (i.e. if ( wildmultcmp ("bl?h.*;*.png", "blah.jpg") )
// Notes: none
// "<reject><caseless>*HAM*;<accept>*.c*;*.h;*.m"; ret = wildcompare ("hamburger.cpp", pattern)
// "<reject><case>*HAM*;<accept>*.c*;*.h;*.m"; ret = wildcompare ("hamburger.cpp", pattern);
// "<reject><case>*HAM*;<accept>*.cpp;*.c*;*.h;*.m"; ret = wildcompare ("hamburger.cpp", pattern);

cbool wildcompare (const char *s, const char *wild_patterns)
{
//	size_t len = strlen (wild);   // Not used
	char *wildbuf = core_strdup (wild_patterns);
	static const char *splitter = ";";
	char *token;
	cbool ret = 0;

	typedef enum {
		eval_rejected = -1,
		eval_undetermined = 0,
		eval_accepted = 1
	} eval_e;

	// DEFAULT: reject-unless --- normally we need something to match to qualify "*.c; *.h" <--- reject unless one of those hits
	eval_e eval =	SWITCH_CASE		(eval_rejected, String_Does_Start_With (wild_patterns, "<reject-unless>") && !!(wild_patterns = &wild_patterns[strlen("<reject-unless>")])	)	// Normal
					SWITCH_CASE		(eval_accepted, String_Does_Start_With (wild_patterns, "<accept-unless>") && !!(wild_patterns = &wild_patterns[strlen("<accept-unless>")])	)	// Uncommon
					SWITCH_DEFAULT	(eval_rejected);													// DEFAULT: reject-unless

	// Order of operation sensitive.  Reject or accept must lead.
//	if (String_Does_Start_With (token, "<reject>")

	eval_e eval_mode = eval_accepted; // By default, we use accepted.

	int casematters_mode = false; // Defaults caseless

	// Get the first token
	for ( token = strtok(wildbuf, splitter); token; token = strtok (NULL, splitter)) {
//		const char *begun_at = token;
		cbool passed_token;
//		int bob = alert ("%s vs. wild token %s", s, token);
		//logd ("%s", token);  //		alert ("atom: %s", token);
//		cbool token_evaluation =
		eval_e mode_changed =	SWITCH_CASE		(eval_rejected, String_Does_Start_With (token, "<reject>") && !!(token = &token[strlen("<reject>")])	)
								SWITCH_CASE		(eval_accepted, String_Does_Start_With (token, "<accept>") && !!(token = &token[strlen("<accept>")])	)
								SWITCH_DEFAULT	(eval_undetermined); // Default:  For the moment


		if (String_Does_Start_With (token, "<case>") && !!(token = &token[strlen("<case>")]))
			casematters_mode = true;
		if (String_Does_Start_With (token, "<caseless>") && !!(token = &token[strlen("<caseless>")]))
			casematters_mode = false;


		if (mode_changed != eval_undetermined)
			eval_mode = mode_changed;

		// For shortcircuit ANY match is approval:
		passed_token = wildcmpx (s, token, casematters_mode);

		if (passed_token && eval_mode == eval_accepted && /*evil set:-->*/ (eval = eval_accepted) ) {
//			alert ("%s was accepted on TOKEN %s\n\n \n\nExpression (%s)", s, token, wild_patterns);
			break; // Accepted immediately
		}
		if (passed_token && eval_mode == eval_rejected && /*evil set:-->*/ (eval = eval_rejected) ) {
//			alert ("%s was rejected on %s\n\nExpression (%s)", s, token, wild_patterns);
			break; // Rejected immediately
		}

	}

//	alert ("string " QUOTED_S " vs (%s) = %d", s, wild_patterns, ret);

	wildbuf = core_free (wildbuf);
	ret = (eval == eval_accepted);
	return ret;
}


char *String_Write_NiceFloatString (char *s_dest, size_t siz, double floatvalue)
{
	c_snprintfc (s_dest, siz, "%f", floatvalue);

	return String_Edit_RemoveTrailingZeros (s_dest); // Also removes period
}


// Short: Command line to argv
// Notes: None.
// Unit Test:


// Modified to accept chars > 127.  If you don't want those, strip them in advance or blank them out.
// ASCII = chars 0-127, where chars > 127 = ANSI codepage 1252
void * Line_Parse_Free (lparse_t *ptr)
{
	return core_free(ptr);
}


// Note this function is only nominally "quote aware"
// Making it more quote aware might make it less comply with
// standardish parsing behavor.
// bob"frog man" >>--- standard parser becomes --> bob "frog man"
// A more atomic parse (we aren't doing here) would be bob"frog man"

lparse_t *Line_Parse_Alloc (const char *s, cbool allow_empty_args)
{
	lparse_t * line = core_calloc (sizeof(lparse_t), 1);
	char *cmdline = line->chopped;
	int n;

	c_strlcpy (line->original, s);
	c_strlcpy (line->chopped, s);

	for (n = 0; n < MAX_ARGS_80; n ++)
		line->args[n] = empty_string;

	while (*cmdline && (line->count < MAX_ARGS_80))
	{
		const char *pre_advance = cmdline;

		// Advance beyond spaces, white space, delete and non-ascii characters
		while (*cmdline && (*cmdline <= SPACE_CHAR_32) )
			cmdline ++;

		// We advanced past spaces and hit terminator.  If we allow empty args, we have an empty final argument.
		if (allow_empty_args && cmdline > pre_advance && cmdline[0] == 0)
		{
			cmdline[-1] = 0; // Turn the whitespace prior into a null.
			line->args[line->count++] = cmdline; // New arg at terminator that is empty.
		}

		switch (*cmdline)
		{
		case 0:  // null terminator
			break;

		case '\"': // quote

			// advance past the quote
			cmdline ++;

			line->args[line->count++] = cmdline;

			// continue until hit another quote or null terminator
			while (*cmdline && *cmdline != '\"')
				cmdline++;

			// Unterminated (*cmdline == 0) is handled fine since we don't copy args.
			break;

		default:
			// check for a comment
			if (cmdline[0] == '/' && cmdline[1] == '/')
			{
				cmdline[0] = 0;
				break;
			}

			line->args[line->count++] = cmdline;

			// Advance until reach space, white space, delete or non-ascii

			while (*cmdline && (*cmdline > SPACE_CHAR_32) )
				cmdline++;
		} // End of switch

		// If more advance the cursor past what should be whitespace
		if (*cmdline)
		{
			*cmdline = 0;
			cmdline ++;
			if (allow_empty_args && *cmdline == 0)
				line->args[line->count++] = cmdline; // New arg at terminator that is empty.
		}

	} // end of while (*cmdline && (line->count < MAX_NUM_ARGS_128))

	return line;
}

/*
typedef struct split_s
{
	int count;
	char **vars;
	int *isnum;
} split_t;
*/


split_t *Split_Free (split_t *split)
{
	// Free each string, then the array holding them and then the split itself
	int i;
	for (i = 0; i < split->count; i ++)
		free (split->vars[i]);

	free (split->vars);	free(split->isnum); free (split);
	return NULL;
}


char *Split_To_String_Alloc (split_t *split, const char *sdelim)
{
	text_a *s = NULL; // Required to be null
	int i;

	txtcat (&s, "%s", split->vars[0]);
	for (i = 1; i < split->count; i ++)
	{
		txtcat (&s, "%s", sdelim);
		txtcat (&s, "%s", split->vars[i]);
	}
	return (char *)s;
}

// Whitespace trims (including spaces) unquoted strings
split_t *Split_Alloc (const char *s, const char *sdelim)
{
	split_t *split_o = calloc (sizeof(split_t), 1);
	char *str_a = strdup (s);
	const char *delim;
	char *cur, *nextcur;
	int count, dlen;

	delim = sdelim;
	for (cur = str_a, dlen = strlen(delim), count = 0; cur; count++, (cur = strstr (cur, delim)), cur = cur ? &cur[dlen] : NULL)
	{
	}

	split_o->count = count;
	split_o->vars = calloc (sizeof(char*), split_o->count);
	split_o->isnum = calloc (sizeof(int), split_o->count);

	delim = sdelim;
	for (cur = str_a, dlen = strlen(delim), count = 0; cur; count++, (cur = strstr (cur, delim)), cur = cur ? &cur[dlen] : NULL)
	{
		if ((nextcur = strstr (cur, delim))!=NULL) nextcur[0] = 0; // Null terminate
		// Null terminated
		{
			char *tmp_a = strdup(cur);
			String_Edit_Trim (tmp_a);
			String_Edit_Unquote (tmp_a);

			split_o->isnum[count] = String_Does_Have_Excel_Numeric_Representation (tmp_a);
			split_o->vars[count] = strdup (tmp_a);
			free (tmp_a);
		}
		// End null terminated
		if (nextcur) nextcur[0] = delim[0]; // Restore it
	}

	free (str_a);
	return split_o;
}

#if 0

// FOIL: Type:TextBox Name:Bob{}\" ?
// Whitespace trims (including spaces) unquoted strings
split_t *Split_EXA6_Alloc (const char *s)
{
	split_t *split_o = calloc (sizeof(split_t), 1);
	char *str_a = strdup (s);
	char *c, *start;

/*
typedef struct split_s
{
	int count;
	char **vars;
	int *isnum;
} split_t;
*/
	int curlybrace_count = 0;
	c = str_a
	while (*c)
	{
		// Advance beyond spaces, white space, delete and non-ascii characters
		// ASCII = chars 0-127, where chars > 127 = ANSI codepage 1252
		while (*c && (*c <= SPACE_CHAR_32 || MAX_ASCII_PRINTABLE_126 <= *c ) ) // Changed to 126, 127 is delete and cannot print
			c ++;

		if (*c == 0)
			break;

	// New argument
		start = c; // New argument
		end = 0;

		// We advance until we hit term, whitespace
		while (*c && end == 0)
		{
			switch (*c)
			{
			case 0:  // null terminator  // Can't happen?
				break;

			case '\"': // quote

				// advance past the quote
				c ++;
				// continue until hit another quote or null terminator
				while (*c && *c != '\"')
					c ++;
				break;

			case '{': // curly brace
				// advance past the brace
				curlybrace_count ++;
				c ++;

				while (curlybrace_count && *c)
				{
					switch (*c)
					{
					case '{': curlybrace_count ++; break;
					case '}': curlybrace_count --; break;
					default: break;
					}
					c ++;
				}
				break;

			default:
				if (*c <= SPACE_CHAR_32 && MAX_ASCII_PRINTABLE_126 <= *c) // Changed to 126, 127 is delete and cannot print
					end = c;
			}
			c ++;
		}
		// We should have end of new word

		memcpy (something, start, end - start + 1); // strdup?  strndup?

			default:
				c++

				// Advance until reach space, white space, delete or non-ascii
				while (*cmdline && (SPACE_CHAR_32 < *cmdline && *cmdline <= MAX_ASCII_PRINTABLE_126 ) )  // Changed from < 127 to <=126 for better clarity
					cmdline++;
			} // End of switch

			// If more advance the cursor past what should be whitespace
			if (*cmdline)
			{
				*cmdline = 0;
				cmdline ++;
			}

		} // end of while (*cmdline && (line->count < MAX_NUM_ARGS_128))

		return line;
	}






	const char *delim;
	char *cur, *nextcur;
	int count, dlen;


	delim = sdelim;
	for (cur = str_a, dlen = strlen(delim), count = 0; cur; count++, (cur = strstr (cur, delim)), cur = cur ? &cur[dlen] : NULL)
	{
	}

	split_o->count = count;
	split_o->vars = calloc (sizeof(char*), split_o->count);
	split_o->isnum = calloc (sizeof(int), split_o->count);

	delim = sdelim;
	for (cur = str_a, dlen = strlen(delim), count = 0; cur; count++, (cur = strstr (cur, delim)), cur = cur ? &cur[dlen] : NULL)
	{
		if ((nextcur = strstr (cur, delim))!=NULL) nextcur[0] = 0; // Null terminate
		// Null terminated
		{
			char *tmp_a = strdup(cur);
			String_Edit_Trim (tmp_a);
			String_Edit_Unquote (tmp_a);

			split_o->isnum[count] = String_Does_Have_Excel_Numeric_Representation (tmp_a);
			split_o->vars[count] = strdup (tmp_a);
			free (tmp_a);
		}
		// End null terminated
		if (nextcur) nextcur[0] = delim[0]; // Restore it
	}

	free (str_a);
	return split_o;
}
#endif


/*
typedef struct spreads_s
{
	int rows;
	int cols;
	int count;
	char **vars;  // Think of vars[rows][cols]
	int **isnum;
	float **nums; // Value representation
} spreads_t;
*/


spreads_t *Spreads_Free (spreads_t *spreads)
{
	// Free each string, then the array holding them and then the split itself
	int i;
	for (i = 0; i < spreads->count; i ++)
		free (spreads->vars[i]);

	free (spreads->vars);  free (spreads->nums); free (spreads->isnum);
	free (spreads);
	return NULL;
}


char *Spreads_To_String_Alloc (spreads_t *spreads, const char *sdelim, const char *slinedelim)
{
	text_a *s = NULL; // Required to be null
	int r, c, cell;

	cell = 0;
	for (r = 0; r < spreads->rows; r ++)
	{
		// Add a line delimiter if not the first row
		if (r != 0)
			txtcat (&s, "%s", slinedelim);

		for (c = 0; c < spreads->cols; c ++)
		{
			// Add a column delimiter if not the first column
			if (c != 0)
				txtcat (&s, "%s", sdelim);

			txtcat (&s, "%s", spreads->vars[cell++]);
		}
	}
	return (char *)s;
}


spreads_t *Spreads_Alloc (const char *s, const char *sdelim, const char *slinedelim)
{
	spreads_t *spreads_o = NULL;
	char *str_a = strdup (s);
	const char *delim;
	char *cur, *nextcur;

	int dlen, count, i;

	int valid;
	int numrows, numcols;

	// Count rows - each iteration is at the start of a row
	valid = 1;
	numcols = 0;
	numrows = 0;
	delim = slinedelim;
	for (cur = str_a, dlen = strlen(delim), count = 0; cur; count++, (cur = strstr (cur, delim)), cur = cur ? &cur[dlen] : NULL)
	{
		if ((nextcur = strstr (cur, delim))!=NULL) nextcur[0] = 0; // Null terminate
		// Begin guaranteed null term --> cur
		{
			// We are at row
			split_t *row_a = Split_Alloc (cur, sdelim);
			int cur_numcols = row_a->count;
			Split_Free (row_a); // Discard

			if (numcols && cur_numcols != numcols)
			{
				// Number of columns deviates from a previous count!!  Invalid for spread;
				valid = 0;
				break;
			}
			else numcols = cur_numcols;
		}
		// End null term as we will put the char back (if we put one on)
		if (nextcur) nextcur[0] = delim[0]; // Restore it
	}

	if (!valid)
	{
		log_fatal ("Inconsistent number of columns for row %d", count);
		free (str_a);
		return NULL;
	}

	// Allocate spreads
	numrows = count;
	count = numrows * numcols;
	spreads_o = calloc (sizeof(spreads_t), 1);
	spreads_o->rows  = numrows;
	spreads_o->cols  = numcols;
	spreads_o->count = spreads_o->rows * spreads_o->cols;
	spreads_o->vars  = calloc (sizeof(char*), spreads_o->count);
	spreads_o->isnum = calloc (sizeof(int), spreads_o->count);
	spreads_o->nums  = calloc (sizeof(float), spreads_o->count);

	// row is count, cell is row * col;
	delim = slinedelim;
	for (cur = str_a, dlen = strlen(delim), count = 0; cur; count++, (cur = strstr (cur, delim)), cur = cur ? &cur[dlen] : NULL)
	{
		if ((nextcur = strstr (cur, delim))!=NULL) nextcur[0] = 0; // Null terminate
		// Begin guaranteed null term --> cur
		{
			int row = count;  // We are at row
			int cell = row * numcols;
			split_t *row_a = Split_Alloc (cur, sdelim);
			for (i = 0; i < row_a->count; i ++)
			{
				spreads_o->vars[cell + i] = strdup (row_a->vars[i]);
				spreads_o->isnum[cell + i] = String_Does_Have_Excel_Numeric_Representation (row_a->vars[i]);
				spreads_o->nums[cell + i] = spreads_o->isnum[cell + i] ? atof(row_a->vars[i]) : 0;
			}
			Split_Free (row_a); // Discard
		}
		// End null term as we will put the char back (if we put one on)
		if (nextcur) nextcur[0] = delim[0]; // Restore it
	}

	free (str_a);
	return spreads_o;
}


char *String_C_Escape_Alloc (const char *s)
{
	text_a *str_o = NULL; // Required

	for (; *s; s++)
	{
		switch (*s)
		{
		case '\a': txtcat (&str_o, "\\" "a");  break;	// \a 	07 	Alarm (Beep, Bell)
		case '\b': txtcat (&str_o, "\\" "b");  break;	// \b 	08 	Backspace
		case '\f': txtcat (&str_o, "\\" "f");  break;	// \f 	0C 	Formfeed
		case '\n': txtcat (&str_o, "\\" "n");  break;	// \n 	0A 	Newline (Line Feed); see notes below
		case '\r': txtcat (&str_o, "\\" "r");  break;	// \r 	0D 	Carriage Return
		case '\t': txtcat (&str_o, "\\" "t");  break;	// \t 	09 	Horizontal Tab
		case '\v': txtcat (&str_o, "\\" "v");  break;	// \v 	0B 	Vertical Tab
		case '\\': txtcat (&str_o, "\\" "\\"); break;	// \\ 	5C 	Backslash
		case '\'': txtcat (&str_o, "\\" "\'"); break;	// \' 	27 	Single quotation mark
		case '\"': txtcat (&str_o, "\\" "\""); break;	// \" 	22 	Double quotation mark
		case '\?': txtcat (&str_o, "\\" "\?"); break;	// \? 	3F 	Question mark

		default:
			// SPACE_CHAR_32 to MAX_ASCII_PRINTABLE_126 print normal ---  Somewhat inefficient
			if ( in_range (SPACE_CHAR_32, *s, MAX_ASCII_PRINTABLE_126 ) )
				txtcat (&str_o, "%c", *s);
			else  txtcat (&str_o, "\\x%02x", (unsigned char)*s);  // otherwise hex encode
			break;
		}
	}
	return (char *)str_o;
}


char *String_C_UnEscape_Alloc (const char *s, reply int *anyerror)
{
	text_a *str_o = NULL; // Required
	int esc_error = 0;

	for (; *s; s++)
	{
		if (*s != '\\')
			txtcat (&str_o, "%c", *s); // Inefficient
		else
		{
			switch (s[1])
			{
			case '\0': txtcat (&str_o, "<<incomplete escape>>"); esc_error = 1; break;  // Error?  We have a NULL.

			case 'a':  txtcat (&str_o, "\a"); break;	// \a 	07 	Alarm (Beep, Bell)
			case 'b':  txtcat (&str_o, "\b"); break;	// \b 	08 	Backspace
			case 'f':  txtcat (&str_o, "\f"); break;	// \f 	0C 	Formfeed
			case 'n':  txtcat (&str_o, "\n"); break;	// \n 	0A 	Newline (Line Feed); see notes below  // newline care
			case 'r':  txtcat (&str_o, "\r"); break;	// \r 	0D 	Carriage Return
			case 't':  txtcat (&str_o, "\t"); break;	// \t 	09 	Horizontal Tab
			case 'v':  txtcat (&str_o, "\v"); break;	// \v 	0B 	Vertical Tab
			case '\\': txtcat (&str_o, "\\"); break;	// \\ 	5C 	Backslash
			case '\'': txtcat (&str_o, "\'"); break;	// \' 	27 	Single quotation mark
			case '\"': txtcat (&str_o, "\""); break;	// \" 	22 	Double quotation mark
			case '\?': txtcat (&str_o, "\?"); break;	// \? 	3F 	Question mark

			case 'x': // Hex
				if (isxdigit(s[2]) && isxdigit(s[3]) )
				{
					char temp[] = {s[2], s[3], 0};
					int val = (int) strtol (temp, NULL, 16); // We use 0 because begins 0x

					txtcat (&str_o, "%c", val);
					s += 3; // 4 character escape (\xf6, etc.) sequence so advance an extra 3
					continue;
				}
				// Error
				txtcat (&str_o, "<<invalid or unsupported 0 escape %d>>", s[2]);
				esc_error = 1;
				break;

			default:   // Something we don't support apparently ...
				txtcat (&str_o, "<<invalid or unsupported escape %d>>", s[1]);
				esc_error = 1;
				break;
			}
			s += 1; // 2 character escape sequence so advance an extra 1
		}
	}
	NOT_MISSING_ASSIGN (anyerror, esc_error);
	return (char *)str_o;
}


/*
URL Encode?

Base 64?


To knock that stuff off why we are focused?



http://base64.sourceforge.net/b64.c


*/

#if 0

SPLIT RAW

/*
typedef struct split_raw_s
{
	const char *int count;
	char **vars;
	int *isnum;
} split_raw_t;
*/


split_t *Split_Free (split_t *split)
{
	// Free each string, then the array holding them and then the split itself
	int i;
	for (i = 0; i < split->count; i ++)
		free (split->vars[i]);

	free (split->vars);	free(split->isnum); free (split);
	return NULL;
}


char *Split_To_String_Alloc (split_t *split, const char *sdelim)
{
	char *s = NULL; // Required to be null
	int i;

	txtcat (&s, split->vars[0]);
	for (i = 1; i < split->count; i ++)
	{
		txtcat (&s, sdelim);
		txtcat (&s, split->vars[i]);
	}
	return s;
}

// Whitespace trims (including spaces) unquoted strings
split_t *Split_Alloc (const char *s, const char *sdelim)
{
	split_t *split_o = calloc (sizeof(split_t), 1);
	char *str_a = strdup (s);
	const char *delim;
	char *cur, *nextcur;
	int count, dlen;

	delim = sdelim;
	for (cur = str_a, dlen = strlen(delim), count = 0; cur; count++, (cur = strstr (cur, delim)), cur = cur ? &cur[dlen] : NULL)
	{
	}

	split_o->count = count;
	split_o->vars = calloc (sizeof(char*), split_o->count);
	split_o->isnum = calloc (sizeof(int), split_o->count);

	delim = sdelim;
	for (cur = str_a, dlen = strlen(delim), count = 0; cur; count++, (cur = strstr (cur, delim)), cur = cur ? &cur[dlen] : NULL)
	{
		if ((nextcur = strstr (cur, delim))!=NULL) nextcur[0] = 0; // Null terminate
		// Null terminated
		{
			char *tmp_a = strdup(cur);
			String_Edit_Trim (tmp_a);
			String_Edit_Unquote (tmp_a);

			split_o->isnum[count] = String_Does_Have_Excel_Numeric_Representation (tmp_a);
			split_o->vars[count] = strdup (tmp_a);
			free (tmp_a);
		}
		// End null terminated
		if (nextcur) nextcur[0] = delim[0]; // Restore it
	}

	free (str_a);
	return split_o;
}

#endif // 0


void txtdelat (text_a **pstring, size_t offset, size_t num)
{
	char *s = (char *)*pstring;
	size_t bufsize = strlen (s) + 1;
	char *s_start = &s[offset];
	char *s_beyond = &s[offset + num]; // 1 beyond, which may be null terminator.
	char *s_term = &s[bufsize - 1];
	size_t copylength = s_term - s_beyond + 1;

	memmove (s_start, s_beyond, copylength);

	//String_Edit_Delete_At ((char *)*pstring, bufsize, offset, num);
	//String_Edit_Range_Delete (s, bufsize, s_start, s_end);
}


// This is unlikely to be much of a bottleneck
void txtfree (text_a **pstring)
{
//	TXTFREE (pstring);
#if 1
	if (!*pstring)
		return;

	free ( (void *)*pstring);
	*pstring = NULL;
#endif
}


// Same as c_snprintf_alloc.
text_a *txtnew (const char *fmt, ...)
{
	VA_EXPAND_ALLOC (text, length, bufsiz, fmt);

//	logd ("txtnew: allocated " QUOTED_S " with bufsize %d", text, bufsiz);

	return text; // VA_EXPAND_ALLOC free not necessary as this is alloc function
}


int txtset (text_a **pstring, const char *fmt, ...)
{
	VA_EXPAND_ALLOC (text, length, bufsiz, fmt);

	txtfree (pstring);
	*pstring = text;
	return bufsiz; // VA_EXPAND_ALLOC free not necessary as this is alloc function, specifically for txtset it is a realloc like action
}


int txtreplace (text_a **pstring, const char *s_find, const char *s_replace)
{
	int num_replaces = -1;
	char *newstring = String_Replace_Len_Count_Alloc (*pstring, s_find, s_replace, NULL, NULL, &num_replaces);
	if (num_replaces) {
//		alert ("num replaces %d of " QUOTED_S " with " QUOTED_S NEWLINE NEWLINE
//				QUOTED_S NEWLINE NEWLINE
//					"%s", num_replaces, s_find, s_replace, *pstring, newstring);
		*pstring = core_free (*pstring);
		*pstring = newstring, newstring = NULL;
	}
	return num_replaces;
}


// Fuck it, no bufsize right now.   Besides it is probably strlen +/-
void txtsetfree (text_a **pstring, char *s_free)
{
	free (*pstring); // Kill mine.
	*pstring = s_free; // Take yours.


//		int bufsiz = txtset (pstring, s_free);
//		free (s_free);
//
//		return bufsiz;
}


void txtcatfree (text_a **pstring, char *s_free)
{
//	char *newstring = NULL;
	if (*pstring == NULL) {
//			int bufsiz = txtcat (pstring, s_free);
//			free (s_free);
//
//			return bufsiz;
		*pstring = s_free;
		// Ideally s_free would be set to NULL but we can't access that memory.

	}
	else
	{
		char *p = NULL;
		int len1 = strlen(*pstring);
		int len2 = strlen(s_free);
		int combined = len1 + len2 + 1; /*room for null*/
		*pstring = realloc (*pstring, combined);

		p = *pstring;
		memcpy (&p[len1], s_free, len2);
		p[combined - 1] = 0; // Null
		free (s_free);
	}

}


// A more optimized version would do what?  Well, a really optimized version would realloc?
// But a super-optimized one would know the buffer size and wouldn't need to strlen even ...
int txtcat (text_a **pstring, const char *fmt, ...)
{
	VA_EXPAND_ALLOC (text, length, bufsiz, fmt);

	bufsiz = txtset (pstring, "%s%s", *pstring ? *pstring : "", text);
	free (text); // VA_EXPAND_ALLOC free
	return bufsiz;
}


int txtprecat (text_a **pstring, const char *fmt, ...)
{
//	char *newstring = NULL;
	VA_EXPAND_ALLOC (text, length, bufsiz, fmt);

	bufsiz = txtset (pstring, "%s%s", text, *pstring ? *pstring : "");
	free (text); // VA_EXPAND_ALLOC free not necessary as this is alloc function
	return bufsiz;
}


// Returns new length.
	/*
	\e	Write an <escape> character.
	\a	Write a <bell> character.
	\b	Write a <backspace> character.
	\f	Write a <form-feed> character.
	\n	Write a <new-line> character.
	\r	Write a <carriage return> character.
	\t	Write a <tab> character.
	\v	Write a <vertical tab> character.
	\'	Write a <single quote> character.
	\\	Write a backslash character.
	*/

// Strips newlines, carriage returns and backspaces,
void String_Edit_To_Single_Line (char *s_edit)
{
	int length = strlen(s_edit);

	const char *temp_a = strdup(s_edit);
	int remaining = length;

	const char *src = temp_a;
	char *dst = s_edit;			// Yes we rewrite

	// Truncate at any new line or carriage return or backspace character
	// BUT convert any whitespace characters that are not actual spaces into spaces.
	//while (*src && dst - cliptext < sizeof out - 1 && *src != '\n' && *src != '\r' && *src != '\b')
	while (*src && remaining > 0 && *src != '\n' && *src != '\r' && *src != '\b')
	{
		if (*src < ' ')
			*dst++ = ' ';
		else *dst++ = *src;
		src++;
		remaining --;
	}
	*dst = 0;
}


const char *String_Get_Word (const char *text, const char *s_find, char *buffer, size_t buffer_size)
{
	int c;

	buffer[0] = 0;

	if (!text)
		return NULL;

	if (!text[0])
		return NULL;

	// Advance past white space
	while ( (c = text[0]) <= ' ') {
		if (c == 0)
			return text;	// return end of string
		text ++;
	}

	{
		const char *next = String_Find_Caseless (text, s_find);
		const char *cursor = text;
		size_t copysize = next - text;
		if (!next) {
			// Copy the rest out
			strlcpy (buffer, text, buffer_size);
			String_Edit_RTrim_Whitespace_Including_Spaces (buffer);
			return &text[strlen(text)]; // Return end of string
		}

		if (copysize >= buffer_size)
			copysize = buffer_size - 1;

		memcpy (buffer, text, copysize);
		buffer[copysize] = 0;

		String_Edit_RTrim_Whitespace_Including_Spaces (buffer);
		return next + strlen(s_find);
	}
}


char *_joindupfz (const char *separator, const char *string1, va_list arglist)
{
	text_a *text = NULL;

	const char *s = string1;

	for ( ; s ; s = va_arg(arglist, char *) /* grab next */  ) {
		if (separator && text)	txtcat (&text, "%s", separator);
		if (1)					txtcat (&text, "%s", s);
	}

	if (!text) text = strdup (""); // Don't return NULL even if no parameters.
	return text;
}



char *catdupfz (const char *text_1, ...)
{
	text_a *text = NULL;
	VA_ARGSCOPY_START (text_1);
	text = _joindupfz ("", text_1, argscopy);
	VA_ARGSCOPY_END;
	return text;
}

char *joindupfz (const char *separator, const char *text_1, ...)
{
	text_a *text = NULL;
	VA_ARGSCOPY_START (text_1);
	text = _joindupfz (separator, text_1, argscopy);
	VA_ARGSCOPY_END;
	return text;
}



const char *nthz (int index, const char *text_1, ...)
{
	const char *s_found = NULL;
	const char *s_arg;

	va_list argptr;
	int n;

	va_start (argptr, text_1); // Initialize

	for (s_arg = text_1, n = 1; !s_found ; s_arg = va_arg (argptr, char *), n ++)
	{
		if (!s_arg) // Hit null
			break;

		if (n == index)
			s_found = s_arg;

	}

	va_end (argptr); // Shutdown

	return s_found;
}


//
int qsort_strcasecmp (const void *a, const void *b)
{
	char *aa = *(char **)a;
	char *bb = *(char **)b;
	return strcasecmp (aa,bb);
}

int qsort_strcmp (const void *a, const void *b)
{
	char *aa = *(char **)a;
	char *bb = *(char **)b;
	return strcmp (aa,bb);
}

int qsort_strcasecmp_rev (const void *a, const void *b)
{
	char *aa = *(char **)a;
	char *bb = *(char **)b;
	return -strcasecmp (aa,bb);
}

int qsort_strcmp_rev (const void *a, const void *b)
{
	char *aa = *(char **)a;
	char *bb = *(char **)b;
	return -strcmp (aa,bb);
}

int printlinef (const char *fmt, ...) //__core_attribute__((__format__(__printf__,1,2);
{
	int result;
	VA_EXPAND_ALLOC (text, length, bufsiz, fmt); // 1024 barrier is NOT be ok for printf replacement, so variable sized

	result = printf ("%s" NEWLINE, text);
	free (text); // free VA_EXPAND_ALLOC allocation.
	return result;  // I guess?
}

// fprintf: If successful, the total number of characters written is returned otherwise, a negative number is returned.
//int fprintf(FILE *stream, const char *format, ...)
int fprintlinef (FILE *stream, const char *fmt, ...) //__core_attribute__((__format__(__printf__,2,3);
{
	int result;
	VA_EXPAND_ALLOC (text, length, bufsiz, fmt); // 1024 barrier is NOT be ok for fprintf replacement, so variable sized

	result = fprintf (stream, "%s" NEWLINE, text);
	free (text); // free VA_EXPAND_ALLOC allocation.
	return result;  // If successful, the total number of characters written is returned otherwise, a negative number is returned.
}


int perrorlinef (const char *fmt, ...)
{
	VA_EXPAND_NEWLINE (text, SYSTEM_STRING_SIZE_1024, fmt); // 1024 barrier is probably ok for error message.
	// Really?
	perror (text); // mingw doesn't recognize this as a noreturn function.  So throws noreturn error.

    exit (1);   // Trick gcc
#ifndef __GNUC__ // noreturn

	return 1;
#endif	// __GNUC__
}

#if 0
int perrorf (const char *fmt, ...)
{
	VA_EXPAND (text, SYSTEM_STRING_SIZE_1024, fmt); // 1024 barrier is probably ok for error message.
	// Really?
	perror (text); // mingw doesn't recognize this as a noreturn function.  So throws noreturn error.

    exit (1);   // Trick gcc
#ifndef __GNUC__ // noreturn

	return 1;
#endif	// __GNUC__
}
#endif // Discouraged

