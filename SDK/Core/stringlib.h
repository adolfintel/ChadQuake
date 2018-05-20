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
// stringlib.h -- extended string functionality

// TODO:  Note ... searching for and comparison of ch > 127 not supported and doesn't work.
// This file is meant to work on ASCII only.

#ifndef __STRING_LIB_H__
#define __STRING_LIB_H__

#include "environment.h"

#include <stdio.h> // fopen, etc.
#include <stdlib.h> // malloc, etc.
#include <stdarg.h> // va_list, etc.

//int perrorf (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2), __noreturn__)); // Hopefully can stay dead?
int printlinef (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2)));
int perrorlinef (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2), __noreturn__));
int fprintlinef (FILE *stream, const char *fmt, ...) __core_attribute__((__format__(__printf__,2,3)));


	#define	 NEWLINE "\n"

	//#define  QUOTED(s) "\"" s "\""
	#define QUOTEDSTR(s) "\"" s "\""  //QUOTEDSTR("maxplayers")

	#define  QUOTED_F	"\"%f\""	//       quoted
	#define  QUOTED_G	"\"%g\""	//       quoted
	#define  QUOTED_D	"\"%d\""	//       quoted

	#define  QUOTED_S	"\"%s\""	//       quoted

// My gut instinct is to carefully limit the use of these ...
	//#define _QUOTED_ 	" \"%s\" "
	//#define _QUOTED 	" \"%s\""
	//#define  QUOTED_ 	 "\"%s\" "

	#define ASCII_A_65					65
	#define ASCII_Z_90					90
	#define TAB_CHAR_9					9
	#define NEWLINE_CHAR_10				10
	#define CARRIAGE_RETURN_CHAR_13		13
	#define SPACE_CHAR_32				32
	#define MAX_ASCII_PRINTABLE_126		126 // TILDE
	#define MAX_ASCII_DELETE_CHAR_127	127 // DELETE CHAR, which might be a key, but isn't really printable.  
												// Depending on situation may be desirable (key input) or undesirable (printing ... usually ...)


#define CORE_STRINGS_VA_ROTATING_BUFFER_BUFSIZ_1024 1024
#define CORE_STRINGS_VA_ROTATING_BUFFERS_COUNT_32 32

char *va (const char *format, ...) __core_attribute__((__format__(__printf__,1,2)));
char *c_strdupf (const char *fmt, ...)__core_attribute__((__format__(__printf__,1,2))) ; // Same except first param returns length

// c_len_strdupf returns length allocated, that's all
char *c_len_strdupf (reply int *len, const char *fmt, ...)__core_attribute__((__format__(__printf__,2,3))) ; // Same except first param returns length

char *String_Replace_Len_Count_Alloc (const char *s, const char *s_find, const char *s_replace, reply int *created_length, reply size_t *created_bufsize, reply int *replace_count);
char *String_Replace_Alloc (const char *s, const char *s_find, const char *s_replace);


#define c_snprintf1(_var,_fmt,_s1) c_snprintfc (_var, sizeof(_var), _fmt, _s1)
#define c_snprintf2(_var,_fmt,_s1,_s2) c_snprintfc (_var, sizeof(_var), _fmt, _s1,_s2)
#define c_snprintf3(_var,_fmt,_s1,_s2,_s3) c_snprintfc (_var, sizeof(_var), _fmt, _s1,_s2,_s3)
#define c_snprintf4(_var,_fmt,_s1,_s2,_s3,_s4) c_snprintfc (_var, sizeof(_var), _fmt, _s1,_s2,_s3,_s4)
#define c_snprintf5(_var,_fmt,_s1,_s2,_s3,_s4,_s5) c_snprintfc (_var, sizeof(_var), _fmt, _s1,_s2,_s3,_s4,_s5)
#define c_snprintf6(_var,_fmt,_s1,_s2,_s3,_s4,_s5,_s6) c_snprintfc (_var, sizeof(_var), _fmt, _s1,_s2,_s3,_s4,_s5,_s6)
#define c_snprintf7(_var,_fmt,_s1,_s2,_s3,_s4,_s5,_s6,_s7) c_snprintfc (_var, sizeof(_var), _fmt, _s1,_s2,_s3,_s4,_s5,_s6,_s7)
#define c_snprintf8(_var,_fmt,_s1,_s2,_s3,_s4,_s5,_s6,_s7,_s8) c_snprintfc (_var, sizeof(_var), _fmt, _s1,_s2,_s3,_s4,_s5,_s6,_s7,_s8)
#define c_snprintf9(_var,_fmt,_s1,_s2,_s3,_s4,_s5,_s6,_s7,_s8,_s9) c_snprintfc (_var, sizeof(_var), _fmt, _s1,_s2,_s3,_s4,_s5,_s6,_s7,_s8,_s9)
#define c_snprintf10(_var,_fmt,_s1,_s2,_s3,_s4,_s5,_s6,_s7,_s8,_s9,_s10) c_snprintfc (_var, sizeof(_var), _fmt, _s1,_s2,_s3,_s4,_s5,_s6,_s7,_s8,_s9,_s10)

#ifndef _DEBUG
#define String_Does_Contain(_s, _find) (strstr(_s, _find) != NULL)

#define String_Does_Match(_s1, _s2) !strcmp(_s1, _s2)
#define String_Does_Match_Caseless(_s1, _s2) !strcasecmp(_s1, _s2)
#define String_Does_Match_Nullproof(_s1, _s2) ( _s1 == _s2 ? 1 : ((_s1 == NULL || _s2 == NULL) ? 0 : !strcmp (_s1, _s2)))

#define String_Does_Not_Match(_s1, _s2) !!strcmp(_s1, _s2)
#define String_Does_Not_Match_Caseless(_s1, _s2) !!strcasecmp(_s1, _s2)

#define String_Does_Not_Start_With(_s, _prefix) !!strncmp(_s, _prefix, strlen(_prefix))
#define String_Does_Not_Start_With_Caseless(_s, _prefix) !!strncasecmp(_s, _prefix, strlen(_prefix))

#define String_Does_Start_With(_s, _prefix) !strncmp(_s, _prefix, strlen(_prefix))
#define String_Does_Start_With_Caseless(_s, _prefix) !strncasecmp(_s, _prefix, strlen(_prefix))

#endif // _DEBUG


#if 0
#ifdef _DEBUG
	#define COMPILE_TIME_ASSERT_SIZE(pred) switch(0){case 0:case pred:;}
#else
	#define COMPILE_TIME_ASSERT_SIZE(pred)
#endif

#define c_strlcpy(_dest, _source) COMPILE_TIME_ASSERT_SIZE((sizeof(_dest) != sizeof(void *))) \
		strlcpy (_dest, _source, sizeof(_dest))
#endif

#define c_strlcpy(_dest, _source) strlcpy (_dest, _source, sizeof(_dest))
#define c_strlcat(_dest, _source) strlcat (_dest, _source, sizeof(_dest))

void String_Command_Argv_To_String (char *cmdline, int numargc, char **argvz, size_t siz);
// Note, the tokenizer counts on the string being available as long as the tokenizer is in use
void String_Command_String_To_Argv (char *cmdline, int *numargc, char **argvz, int maxargs);

#define ARG_BUCKETS_COUNT_64 64
struct arg_buckets_64_s
{
	char	cmdline[SYSTEM_STRING_SIZE_1024];
	char	*argvs[ARG_BUCKETS_COUNT_64];
	int		argcount;
};

void String_To_Arg_Buckets (struct arg_buckets_64_s *argbucket, const char *s);


int String_Count_Char (const char *s, int ch_findchar);
int String_Count_Newlines (const char *s);
int String_Count_String (const char *s, const char *s_find);
int String_Count_Words (const char *s);
int String_Count_First_Word_Length (const char *s);

#ifdef _DEBUG // Not for release version, we use a macro

int String_Does_Contain (const char *s, const char *s_find);

#endif // _DEBUG ... release version uses macro

int String_Does_Contain_Spaces (const char *s);
int String_Does_List_Delimited_Contain (const char *s, const char *s_find, int ch_delimiter);
int String_Does_End_With_Caseless (const char *s, const char *s_suffix);
int String_Does_End_With (const char *s, const char *s_suffix);
int String_Does_Have_Excel_Numeric_Representation (const char *s);
int String_Does_Have_Lowercase (const char *s);
int String_Does_Have_Quotes(const char *s); // Name not ideal, but consistent
int String_Does_Have_Uppercase (const char *s);

// Broke the naming convention, haha!  Does it a bit verbose anyway.
cbool String_Is_Only_Alpha_Numeric_Plus_Charcode (const char *s, int charcode);

#ifdef _DEBUG // Not for release version, we use a macro

int String_Does_Match (const char *s1, const char *s2);
int String_Does_Match_Caseless (const char *s1, const char *s2);
int String_Does_Match_Nullproof (const char *s1, const char *s2);
int String_Does_Not_Match (const char *s1, const char *s2);
int String_Does_Not_Match_Caseless (const char *s1, const char *s2);
int String_Does_Not_Start_With (const char *s, const char *s_prefix);
int String_Does_Not_Start_With_Caseless (const char *s, const char *s_prefix);

int String_Does_Start_With (const char *s, const char *s_prefix);
int String_Does_Start_With_Caseless (const char *s, const char *s_prefix);

#endif // _DEBUG ... release version uses macro

char *String_Edit_Delete_At (char *s_edit, size_t s_size, size_t offset, size_t num);
char *String_Edit_For_Stdout_Ascii (char *s_edit);
void String_Edit_To_Single_Line (char *s_edit); // Slightly different than String_Edit_For_Stdout_Ascii


int String_Edit_Insert_At (char* s_edit, size_t s_size, const char* s_insert, size_t offset);
char *String_Edit_LTrim_Whitespace_Excluding_Spaces (char *s_edit);
char *String_Edit_LTrim_Whitespace_Including_Spaces (char *s_edit);
char *String_Edit_RTrim_Whitespace_Excluding_Spaces (char *s_edit);
char *String_Edit_RTrim_Whitespace_Including_Spaces (char *s_edit);
char *String_Edit_Range_Delete (char *s_edit, size_t s_size, const char *s_start, const char *s_end);
char *String_Edit_Remove_End (char *s_edit);
char *String_Edit_RemoveTrailingSpaces (char *s_edit);
char *String_Edit_RemoveTrailingZeros (char *s_edit); // 2.000 -> 2
char *String_Edit_Replace (char *s_edit, size_t s_size, const char *s_find, const char *s_replace);
char *String_Edit_Replace_Memory_Constant (char *s_edit, size_t s_size, const char *s_find, const char *s_replace);
char *String_Edit_Replace_Token_Array (char *s_edit, size_t s_size, const char **replace_tokens2);


char *String_Edit_Replace_Char (char *s_edit, int ch_find, int ch_replace, reply int *outcount);
char *String_Edit_Repeat (char *s_edit, size_t s_size, const char *s_repeat, int count);

char *String_Edit_To_ProperFloat (char *s_edit); // Examples: 2.000 -> 2.0, 2.1000 -> 2.1
char *String_Edit_To_Lower_Case (char *s_edit);
char *String_Edit_To_Proper_Case (char *s_edit);
char *String_Edit_To_Upper_Case (char *s_edit);
char *String_Edit_Reverse (char *s_edit);
char *String_Edit_Trim (char *s_edit); // memmoves
char *String_Edit_UnQuote (char *s_edit); // memmoves
char *String_Edit_Whitespace_To_Space (char *s_edit);
char *String_Find (const char *s, const char *s_find);
char *String_Find_Caseless (const char *s, const char *s_find);
char *String_Find_Skip_Past (const char *s, const char *s_find);
char *String_Find_Reverse (const char *s, const char *s_find);
char *String_Find_Char (const char *s, int ch_findchar);
char *String_Find_Char_Nth_Instance (const char *s, int ch_findchar, int n_instance);
char *String_Find_Char_Reverse (const char *s, int ch_findchar);
char *String_Find_End (const char *s);
char *String_Length_Copy (char *s_dest, size_t siz, const char *src_start, size_t length);
int String_Pos_Find (const char *s, const char *s_find);
int String_Pos_Find_Char (const char *s, int ch_findchar);
char *String_Range_Copy (char *s_dest, size_t siz, const char *src_start, const char *src_end);
int String_Range_Count_Char (const char *s_start, const char *s_end, int ch_findchar);

char *String_Range_Find_Char (const char *s_start, const char *s_end, int ch_findchar);
char *String_Range_Find_Char_Reverse (const char *s_start, const char *s_end, int ch_findchar);
char *String_Repeat_Alloc (char ch, int count);

// Duplicates a substring String_Range_Dup_Alloc("john", 1, 2) = "oh" // 0 based like any proper C string function would be.
char *String_Range_Dup_Alloc(const char *s_start, const char *s_end);

char *String_Skip_Char (const char *s, int ch_findchar);
char *String_Skip_Char_Reverse (const char *s, int ch_findchar);
char *String_Skip_WhiteSpace_Excluding_Space (const char *s);
char *String_Skip_WhiteSpace_Including_Space (const char *s);
char *String_Skip_NonWhiteSpace (const char *s);

char *String_Write_NiceFloatString (char *s_dest, size_t siz, double floatvalue);

int String_To_Array_Index (const char *s, const char *s_array[]);
int String_To_Array_Index_Caseless (const char *s, const char *s_array[]);
int String_List_Match (const char *s, const char *s_find, int ch_delim);

#ifdef CORE_NEEDS_STRLCPY_STRLCAT
size_t strlcpy(char *dst, const char *src, size_t siz);
size_t strlcat(char *dst, const char *src, size_t siz);
#endif // CORE_NEEDS_STRLCPY_STRLCAT

#ifdef CORE_NEEDS_STRRSTR
// The strstr() function finds the last occurrence of the substring
char *strrstr(const char *s1, const char *s2);
#endif // CORE_NEEDS_STRRSTR

#ifdef CORE_NEEDS_STRCASESTR
//The strcasestr() function is similar to strstr(), but ignores the case of both strings.
char * strcasestr(const char *s, const char *find);
#endif // CORE_NEEDS_STRCASESTR


#ifdef CORE_NEEDS_STRNLEN
size_t strnlen (const char *string, size_t maxlen);
#endif // CORE_NEEDS_STRNLEN

#ifdef CORE_NEEDS_STRNDUP
char * strndup (const char *s, size_t n);
#endif // CORE_NEEDS_STRNDUP


#ifdef CORE_NEEDS_STRPTIME
// Example: strptime("6 Dec 2001 12:33:45", "%d %b %Y %H:%M:%S", &tm)
// time_t t:  month is 0 to 11, year begins at 1900 (add 1900 for actual year), day is day
// t = mktime(&tm); must be used to get weekday and day of year
char *strptime(const char *buf, const char *fmt, struct tm *tm);
#endif // CORE_NEEDS_STRPTIME


#ifdef CORE_NEEDS_VSNPRINTF_SNPRINTF
	int c_vsnprintf (char * s, size_t n, const char *fmt, va_list arg); // Visual Studio deficiency in some versions
	int c_snprintfc (char * s, size_t n, const char *fmt, ...) __core_attribute__((__format__(__printf__,3,4))) ;
#else
	#define c_vsnsprintf vsnprintf
	#define c_snprintfc snprintf
#endif // CORE_NEEDS_VSNPRINTF_SNPRINTF



// multi-wildcard compare: "<reject><caseless>*HAM*;<accept>frog??.c*;*.h;*.m"
// <case>, <caseless> set mode
// <reject>, <accept> set mode
// order of evaluation matters!  And add a <accept>* to end of an all reject sequence.
cbool wildcompare (const char *s, const char *wild_patterns);

//String_Test_Reject (test, "<valid:alpha,numeric,space>");
// <validchars:alpha,numeric,space>
// <notempty>, <notrim>

// Do not like enough for them to be widely available
//#define isnotvisibleascii(c) ((c) <= 32 || 127 <=(c)) // This actually already includes 127 delete char, good job
//#define iswhitespace(c) ((c) <= 32) // What about 127 delete char? // Going for the traditional definition of whitespace here I guess.  I dis


// Best use:  VA_EXPAND (msg, SYSTEM_STRING_SIZE_1024, fmt);  // Don't use this for text, just printfs and such.

#define VA_EXPAND(_text, _len, _fmt) \
	char		_text[_len]; \
	va_list		argptr; \
	va_start	(argptr, _fmt); \
	c_vsnprintf (_text, sizeof(_text), _fmt, argptr); \
	va_end		(argptr)

// Best use:  VA_EXPAND_NEWLINE (msg, SYSTEM_STRING_SIZE_1024, fmt);
#define VA_EXPAND_NEWLINE(_text, _len, _fmt) \
	char		_text[_len]; \
	va_list		argptr; \
	va_start	(argptr, _fmt); \
	c_vsnprintf (_text, sizeof(_text) - 1 /* assured space for newline */, _fmt, argptr); \
	va_end		(argptr); \
	c_strlcat	(_text, NEWLINE)

// Best use:  VA_EXPAND_ALLOC (text, length, bufsiz, fmt); _length_vsnprintf
#ifdef PLATFORM_WINDOWS
	#define VA_EXPAND_ALLOC(_text, _len, _siz16, _fmt) \
		char		*_text; \
		int			_len; \
		size_t		_siz16; \
		va_list		argptr; \
		va_start	(argptr, _fmt); \
		_text =		_length_vsnprintf (false /* not a test */, &_len, &_siz16, _fmt, argptr); \
		va_end		(argptr)
#else
	#define VA_EXPAND_ALLOC(_text, _len, _siz16, _fmt) \
		char		*_text; \
		int			_len; \
		size_t		_siz16; \
		va_list		argptr; \
		va_start	(argptr, _fmt); \
		va_list		copy; \
		va_copy		(copy, argptr); \
		_text =		_length_vsnprintf (false /* not a test */, &_len, &_siz16, _fmt, copy); \
		va_end		(copy); \
		va_end		(argptr)
#endif


// environment.h
//
//#define MAX_CMD_256 256
//#define MAX_ARGS_80	80

typedef struct
{
	char		original[MAX_CMD_256];
	char		chopped[MAX_CMD_256];
	void 		*source;	// user defined behavior
	int			count;
	const char	*args[MAX_ARGS_80]; // pointers to chopped

} lparse_t;

lparse_t *Line_Parse_Alloc (const char *s, cbool allow_empty_args); // allow_empty_args can allow a blank final parameter
void *Line_Parse_Free (lparse_t *ptr);
// Parse.  Returns NULL when no more words.  Trims each word.
//char word[256];
//const char *cursor, *mystring = cursor = "  dpmaster.deathmask.net:27950 , dpmaster.tchr.no:27950  ";
//while ( (cursor = String_Get_Word (cursor, ",", word, sizeof(word))) ) {
//	alert1 (word);
//	cursor=cursor;
//}

const char *String_Get_Word (const char *text, const char *s_find, char *buffer, size_t buffer_size);

typedef struct split_s
{
	int count;
	char **vars;
	int *isnum;
} split_t;


split_t *Split_Alloc (const char *s, const char *sdelim);
char *Split_To_String_Alloc (split_t *split, const char *sdelim);
split_t *Split_Free (split_t *split);

split_t *Split_EXA6_Alloc (const char *s); // STRICT.  Splits on spaces that are not inside a quote.
// E1: Type:TextBox Text:"Mary had a little lamb" Locked:True
// 1) Type:TextBox
// 2) Text:"Mary had a little lamb"
// 3) Locked:True
//
// E2:  Type:ListBox List:{frogs,dogs,"dig dogger"} Sorted:True
// 1) Type:ListBox
// 2) List:{frogs,dogs,"dig dogger"}
// 3) Sorted:True

// Spreads = Simple Spreadsheet

typedef struct spreads_s
{
	int rows;
	int cols;
	int count;
	char **vars;  // Think of vars[rows][cols]
	int *isnum;
	float *nums; // Value representation
} spreads_t;

spreads_t *Spreads_Free (spreads_t *spreads);
char *Spreads_To_String_Alloc (spreads_t *spreads, const char *sdelim, const char *slinedelim);
spreads_t *Spreads_Alloc (const char *s, const char *sdelim, const char *slinedelim);

// I believe this work, are important but they are in an #if 0 block.
// I have a feeling these will get some attention soon.
// Quoted string values can't be done without them!

char *String_C_Escape_Alloc (const char *s);
char *String_C_UnEscape_Alloc (const char *s, reply int *anyerror);

// http://stackoverflow.com/questions/1135841/c-multiline-string-literal
/* Example of newlines escaped into a string literal
const char *text2 =
  "Here, on the other hand, I've gone crazy \
  and really let the literal span several lines, \
  without bothering with quoting each line's \
  content. This works, but you can't indent.";
*/


// Will add a trailing empty line after a newline.

#define text_a char // It wants to be const char, but char is more flexible

// The idea of these having a standard return value is shrinking.
text_a *txtnew (const char *fmt, ...) __core_attribute__((__format__(__printf__,1,2))) ;
int txtset (text_a **pstring, const char *fmt, ...) __core_attribute__((__format__(__printf__,2,3))) ;
void txtsetfree (text_a **pstring, char *s_free);
int txtcat (text_a **pstring, const char *fmt, ...) __core_attribute__((__format__(__printf__,2,3))) ;;
void txtcatfree (text_a **pstring, char *s_free); // This will free the recipient string.
int txtreplace (text_a **pstring, const char *s_find, const char *s_replace); // This returns # of replaces.

int txtprecat (text_a **pstring, const char *fmt, ...) __core_attribute__((__format__(__printf__,2,3))) ;
void txtfree (text_a **pstring); // Frees a pointer to a string.  If already null, does nothing.
void txtdelat (text_a **pstring, size_t offset, size_t num); // Removes a substring

#undef text_a


char *_length_vsnprintf (cbool just_test, reply int *created_length, reply size_t *created_bufsize, const char *fmt, va_list args);


/*

  // Future use.  Maybe.  Semi-sortable (the high 2 bits) but don't sort using that because anything starting with same first 2 gets same.
  // Tested.  Case insensitive.  Int comparable, Z > A.
unsigned String_Cheap_Hash_Case_Insensitive (const char *s)
{
	byte len = (strlen(s) & 255); // 0xFF
	byte ch1 = toupper(s[0]);
	byte ch2 = len > 1 ? toupper(s[1]) : 0;
	int n, sum;
	byte crc8;
	int result;
	for (n = 0; n < len; n ++)
		sum += toupper(s[n]);

	crc8 = sum & 255;
	result = Color_From_Bytes (crc8, len, ch1, ch2);
	logd ("%s : hash ... %c %c %d %d ... = %d or 0x%08x", s, ch1, ch2, len, crc8, result, result);
	return result;
}

  */


const char *nthz (int index, const char *text_1, ...) __core_attribute__((sentinel));

char *catdupfz (const char *text_1, ...) __core_attribute__((sentinel));
char *joindupfz (const char *separator, const char *text_1, ...) __core_attribute__((sentinel));

char *_joindupfz (const char *separator, const char *string1, va_list arglist); // External functions need access
// nthz
char *Array_Join_Alloc (const char **ray, const char *separator, const char *prefix);


int qsort_strcasecmp (const void *a, const void *b);
int qsort_strcmp (const void *a, const void *b);
int qsort_strcasecmp_rev (const void *a, const void *b);
int qsort_strcmp_rev (const void *a, const void *b);

#endif	// ! __STRING_LIB_H__

