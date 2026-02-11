/************************************************************************/
/* Text-DoubleMetaphone							*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright 2000, Maurice Aubrey <maurice@hevanet.com>.		*/
/* All rights reserved.							*/
/* 									*/
/* This code is copied for redistribution with modification, from the	*/
/* gitpan/Text-DoubleMetaphone implementation on GitHub (1), which is	*/
/* under the following license.						*/
/* 									*/
/*    This code is based heavily on the C++ implementation by Lawrence	*/
/*    Philips and incorporates several bug fixes courtesy of Kevin	*/
/*    Atkinson <kevina@users.sourceforge.net>.				*/
/* 									*/
/*    This module is free software; you may redistribute it and/or	*/
/*    modify it under the same terms as Perl itself.			*/
/* 									*/
/* A summary of the relevant content from https://dev.perl.org/licenses	*/
/* has been included below for the convenience of the reader. This	*/
/* information was collected and saved on September 5th, 2025 and may	*/
/* differ from current information. For the most up to date copy of	*/
/* this information, please use  the link provided above.		*/
/* 									*/
/*    Perl5 is Copyright © 1993 and later, by Larry Wall and others.	*/
/* 									*/
/*    It is free software; you can redistribute it and/or modify it	*/
/*    under the terms of either:					*/
/* 									*/
/*    a) the GNU General Public License (2) as published by the Free	*/
/*	 Software Foundation (3); either version 1 (2), or (at your	*/
/*	 option) any later version (4), or				*/
/* 									*/
/*    b) the "Artistic License" (5).					*/
/* 									*/
/* Citations:								*/
/*    1: https://github.com/gitpan/Text-meta_double_metaphone		*/
/*    2: https://dev.perl.org/licenses/gpl1.html			*/
/*    3: http://www.fsf.org						*/
/*    4: http://www.fsf.org/licenses/licenses.html#GNUGPL		*/
/*    5: https://dev.perl.org/licenses/artistic.html			*/
/* 									*/
/* Centrallix is published under the GNU General Public License,	*/
/* satisfying the above requirement. A summary of this is included	*/
/* below for the convenience of the reader.				*/
/* 									*/
/* This program is free software; you can redistribute it and/or modify	*/
/* it under the terms of the GNU General Public License as published by	*/
/* the Free Software Foundation; either version 2 of the License, or	*/
/* (at your option) any later version.					*/
/* 									*/
/* This program is distributed in the hope that it will be useful,	*/
/* but WITHOUT ANY WARRANTY; without even the implied warranty of	*/
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*/
/* GNU General Public License for more details.				*/
/* 									*/
/* You should have received a copy of the GNU General Public License	*/
/* along with this program; if not, write to the Free Software		*/
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA		*/
/* 02111-1307  USA							*/
/* 									*/
/* A copy of the GNU General Public License has been included in this	*/
/* distribution in the file "COPYING".					*/
/* 									*/
/* Module:	double_metaphone.c, double_metaphone.h			*/
/* Author:	Maurice Aubrey and Israel Fuller			*/
/* Description:	This module implements a "sounds like" algorithm by	*/
/* 		Lawrence Philips which he published in the June, 2000	*/
/* 		issue of C/C++ Users Journal. Double Metaphone is an	*/
/* 		improved version of the original Metaphone algorithm	*/
/* 		written by Philips'. This implementaton was written by	*/
/* 		Maurice Aubrey for C/C++ with bug fixes provided by	*/
/* 		Kevin Atkinson. It was revised by Israel Fuller to	*/
/* 		better align with the Centrallix coding style and	*/
/* 		standards so that it could be included here.		*/
/************************************************************************/

/*** Note to future programmers reading this file (by Israel Fuller):
 *** 
 *** This file was copied from a GitHub Repo with proper licensing (in case
 *** you didn't read the legal stuff above), so feel free to check it out.
 *** 
 *** As for this code, I've modified it to use styling and memory allocation
 *** consistent with the rest of the Centrallix codebase. Also, I have added
 *** documentation comments and extensive test cases (at the end of the file),
 *** however, these reflect my own (possibly incorrect) understanding, which
 *** might not line up with the original author.
 *** 
 *** To be honest, though, trying to make this code as readable as possible
 *** was very challenging due to all the messy boolean algebra. If there is
 *** ever a professional linguist reading this, please factor out some of the
 *** logic into local variables with descriptive names so that the rest of us
 *** can read this code without our eyes glazing over.
 *** 
 *** If you have any questions, please feel free to reach out to me or Greg.
 *** 
 *** Original Source: https://github.com/gitpan/Text-meta_double_metaphone
 ***/

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/*** If running in a testing environment, newmalloc is not
 *** available, so we fall back to default C memory allocation.
 ***/
#ifndef TESTING
#include "cxlib/newmalloc.h"
#define META_MALLOC(size) nmSysMalloc(size)
#define META_REALLOC(ptr, size) nmSysRealloc(ptr, size)
#define META_FREE(ptr) nmSysFree(ptr)
#else
#include <stdlib.h>
#define META_MALLOC(size) malloc(size)
#define META_REALLOC(ptr, size) realloc(ptr, size)
#define META_FREE(ptr) free(ptr)
#endif

/*** Helper function to handle checking for failed memory allocation
 *** Author: Israel Fuller.
 *** 
 *** @param ptr Pointer to the memory that should be allocated.
 *** @param fname The name of the function invoked to allocate memory.
 *** @param size The amount of memory being allocated.
 *** @returns The pointer, for chaining.
 ***/
void*
meta_check_allocation(void* ptr, const char* fname, const size_t size)
    {
	if (ptr == NULL)
	    {
	    /** Create the most descriptive error message we can. **/
	    char error_buf[BUFSIZ];
	    snprintf(error_buf, sizeof(error_buf), "exp_double_metaphone.c: Fail - %s(%lu)", fname, size);
	    perror(error_buf);
	    
	    // Throw error for easier locating in a debugger.
	    fprintf(stderr, "Program will now crash.\n");
	    assert(0);
	    }
    
    return ptr;
    }

/** Malloc shortcut macros. **/
#define SAFE_MALLOC(size) \
    ({ \
	const size_t sz = (size); \
	memset(meta_check_allocation(META_MALLOC(sz), "META_MALLOC", sz), 0, sz); \
    })
#define SAFE_REALLOC(ptr, size) \
    ({ \
	const size_t sz = (size); \
	meta_check_allocation(META_REALLOC(ptr, sz), "META_REALLOC", sz); \
    })

typedef struct
    {
    char* str;
    size_t length;
    size_t bufsize;
    int free_str_on_destroy;
    }
MetaString;

/*** Allocates a new MetaString.
 *** 
 *** @param init_str The initial size of the string.
 *** @returns The new MetaString.
 ***/
MetaString*
meta_new_string(const char* init_str)
    {
    MetaString *s;
    char empty_string[] = "";
    
	s = (MetaString*)SAFE_MALLOC(sizeof(MetaString));
	
	if (init_str == NULL)
	    init_str = empty_string;
	
	s->length = strlen(init_str);
	/** Preallocate a bit more for potential growth. **/
	s->bufsize = s->length + 7u;
	
	s->str = (char*)SAFE_MALLOC(s->bufsize * sizeof(char));
	
	strncpy(s->str, init_str, s->length + 1);
	s->free_str_on_destroy = 1;
    
    return s;
    }

/*** Frees a MetaString.
 *** 
 *** @param s The MetaString.
 ***/
void
meta_destroy_string(MetaString* s)
    {
	if (s == NULL)
	    return;
	
	if (s->free_str_on_destroy && s->str != NULL)
	    META_FREE(s->str);
	
	META_FREE(s);
    
    return;
    }

/*** Increases a MetaString's buffer size.
 *** 
 *** @param s The MetaString* being modified.
 *** @param chars_needed Minimum number of characters to increase buffer size.
 ***/
void
meta_increase_buffer(MetaString* s, const size_t chars_needed)
    {
	s->bufsize += chars_needed + 8u;
	s->str = SAFE_REALLOC(s->str, s->bufsize * sizeof(char));
    
    return;
    }

/*** Convert all characters of a MetaString to uppercase.
 *** 
 *** @param s The MetaString being modified.
 ***/
void
meta_make_upper(MetaString* s)
    {
	for (char* i = s->str; i[0] != '\0'; i++)
	    *i = (char)toupper(*i);
    
    return;
    }

/*** @param s The MetaString being checked.
 *** @param pos The character location to check within the MetaString.
 *** @returns 1 if the location is out of bounds for the MetaString,
 ***          0 otherwise.
 ***/
bool
meta_is_out_of_bounds(MetaString* s, unsigned int pos)
    {
    return (s->length <= pos);
    }

/*** Checks if a character in a MetaString is a vowel.
 *** 
 *** @param s The MetaString being checked.
 *** @param pos The character location to check within the MetaString.
 ***/
bool
meta_is_vowel(MetaString* s, unsigned int pos)
    {
	if (meta_is_out_of_bounds(s, pos)) return 0;
	
	const char c = *(s->str + pos);
    
    return ((c == 'A') || (c == 'E') || (c == 'I') ||
	    (c == 'O') || (c == 'U') || (c == 'Y'));
    }

/*** Search a MetaString for "W", "K", "CZ", or "WITZ", which indicate that the
 *** string is Slavo Germanic.
 *** 
 *** @param s The MetaString to be searched.
 *** @returns 1 if the MetaString is Slavo Germanic, or 0 otherwise. 
 ***/
bool
meta_is_slavo_germanic(MetaString* s)
    {
    return (strstr(s->str, "W") != NULL)
	|| (strstr(s->str, "K") != NULL)
	|| (strstr(s->str, "CZ") != NULL)
	|| (strstr(s->str, "WITZ") != NULL);
    }

/*** @param s The MetaString being checked.
 *** @param pos The character location to check within the MetaString.
 *** @returns The character at the position in the MetaString, or
 ***          '\0' if the position is not in the MetaString.
 ***/
char
meta_get_char_at(MetaString* s, unsigned int pos)
    {
    return (meta_is_out_of_bounds(s, pos)) ? '\0' : ((char) *(s->str + pos));
    }

/*** Checks for to see if any of a list of strings appear in a the given
 *** MetaString after the given start position.
 *** 
 *** @attention - Note that the START value is 0 based.
 *** 
 *** @param s The MetaString being modified.
 *** @param start The zero-based start of at which to begin searching
 *** 	within the MetaString.
 *** @param length The length of the character strings being checked.
 *** @returns 1 if any of the character sequences appear after the start
 *** 	in the MetaString and 0 otherwise.
 ***/
bool
meta_is_str_at(MetaString* s, unsigned int start, ...)
    {
    va_list ap;
    
	/** Should never happen. **/
	if (meta_is_out_of_bounds(s, start))
	    return 0;
	
	const char* pos = (s->str + start);
	va_start(ap, start);
	
	char* test;
	do
	    {
	    test = va_arg(ap, char*);
	    if (*test && (strncmp(pos, test, strlen(test)) == 0))
		return true;
	    }
	while (test[0] != '\0');
	
	va_end(ap);
    
    return false;
    }

/*** Adds a string to a MetaString, expanding the MetaString if needed.
 *** 
 *** @param s The MetaString being modified.
 *** @param new_str The string being added.
 ***/
void
meta_add_str(MetaString* s, const char* new_str)
    {
	if (new_str == NULL)
	    return;
	
	const size_t add_length = strlen(new_str);
	if ((s->length + add_length) > (s->bufsize - 1))
	    meta_increase_buffer(s, add_length);
	
	strcat(s->str, new_str);
	s->length += add_length;
    
    return;
    }
    
/*** Computes double metaphone.
 *** 
 *** Example Usage:
 *** ```c
 *** char* primary_code;
 *** char* secondary_code;
 *** meta_double_metaphone(input, &primary_code, &secondary_code);
 *** ```
 *** 
 *** @param str The string to compute.
 *** @param primary_code A pointer to a buffer where the pointer to a string
 ***	containing the produced primary code will be stored.
 *** @param secondary_code A pointer to a buffer where the pointer to a string
 ***	containing the produced secondary code will be stored.
 ***/
void
meta_double_metaphone(const char* str, char** primary_code, char** secondary_code)
    {
    size_t length;
    
	if (primary_code == NULL)
	    {
	    fprintf(stderr, "Warning: Call to meta_double_metaphone() is missing a pointer to store primary code.\n");
	    return;
	    }
	
	if (secondary_code == NULL)
	    {
	    fprintf(stderr, "Warning: Call to meta_double_metaphone() is missing a pointer to store secondary code.\n");
	    return;
	    }
    
	if (str == NULL || (length = strlen(str)) == 0u)
	    {
	    fprintf(stderr, "Warning: Call to meta_double_metaphone() with invalid string.\n");
	    
	    /** Double Metaphone on an invalid string yields two empty strings. **/
	    *primary_code = (char*)SAFE_MALLOC(sizeof(char));
	    *secondary_code = (char*)SAFE_MALLOC(sizeof(char));
	    return;
	    }
	unsigned int current = 0;
	unsigned int last = (unsigned int)(length - 1);
	
	/** Pad original so we can index beyond end. **/
	MetaString* original = meta_new_string(str);
	meta_make_upper(original);
	meta_add_str(original, "     ");
	
	MetaString* primary = meta_new_string("");
	MetaString* secondary = meta_new_string("");
	primary->free_str_on_destroy = 0;
	secondary->free_str_on_destroy = 0;
	
	/** Skip these if they are at start of a word. **/
	if (meta_is_str_at(original, 0, "GN", "KN", "PN", "WR", "PS", ""))
	    current += 1;
	
	/** Initial 'X' is pronounced 'Z' e.g. 'Xavier' **/
	const char first_char = meta_get_char_at(original, 0);
	if (first_char == 'X')
	    {
	    meta_add_str(primary, "S"); /* 'Z' maps to 'S' */
	    meta_add_str(secondary, "S");
	    current += 1;
	    }
	
	/** Precomputing this is useful. **/
	const bool is_slavo_germanic = meta_is_slavo_germanic(original);
	
	/** Main loop. **/
	while (current < length)
	    {
	    const char cur_char = meta_get_char_at(original, current);
	    const char next_char = meta_get_char_at(original, current + 1);
	    switch (cur_char)
		{
		case 'A':
		case 'E':
		case 'I':
		case 'O':
		case 'U':
		case 'Y':
		    {
		    if (current == 0)
			{
			/** All init vowels now map to 'A'. **/
			meta_add_str(primary, "A");
			meta_add_str(secondary, "A");
			}
		    current += 1;
		    break;	
		    }
		
		case 'B':
		    {
		    /** "-mb", e.g", "dumb", already skipped over... **/
		    meta_add_str(primary, "P");
		    meta_add_str(secondary, "P");
		    
		    current += (next_char == 'B') ? 2 : 1;
		    break;
		    }
	    
		case 'C':
		    {
		    /** Various germanic. **/
		    if (
			(current > 1)
			&& !meta_is_vowel(original, current - 2)
			&& meta_is_str_at(original, (current - 1), "ACH", "")
			&& meta_get_char_at(original, current + 2) != 'I'
			&& (
			    meta_get_char_at(original, current + 2) != 'E'
			    || meta_is_str_at(original, (current - 2), "BACHER", "MACHER", "")
			)
		       )
			{
			meta_add_str(primary, "K");
			meta_add_str(secondary, "K");
			current += 2;
			break;
			}
		    
		    /** Special case 'caesar' **/
		    if (current == 0 && meta_is_str_at(original, current, "CAESAR", ""))
			{
			meta_add_str(primary, "S");
			meta_add_str(secondary, "S");
			current += 2;
			break;
			}
		
		    /** Italian 'chianti' **/
		    if (meta_is_str_at(original, current, "CHIA", ""))
			{
			meta_add_str(primary, "K");
			meta_add_str(secondary, "K");
			current += 2;
			break;
			}
		    
		    if (meta_is_str_at(original, current, "CH", ""))
			{
			/** Find 'michael' **/
			if (current > 0 && meta_is_str_at(original, current, "CHAE", ""))
			    {
			    meta_add_str(primary, "K");
			    meta_add_str(secondary, "X");
			    current += 2;
			    break;
			    }
			
			/** Greek roots e.g. 'chemistry', 'chorus' **/
			if (
			    current == 0
			    && meta_is_str_at(original, (current + 1), "HOR", "HYM", "HIA", "HEM", "HARAC", "HARIS", "")
			    && !meta_is_str_at(original, 0, "CHORE", "")
			   )
			    {
			    meta_add_str(primary, "K");
			    meta_add_str(secondary, "K");
			    current += 2;
			    break;
			    }
			
			/** Germanic, greek, or otherwise 'ch' for 'kh' sound. */
			if (
			    meta_is_str_at(original, 0, "SCH", "VAN ", "VON ", "")
			    /** 'architect but not 'arch', 'orchestra', 'orchid' **/
			    || meta_is_str_at(original, (current - 2), "ORCHES", "ARCHIT", "ORCHID", "")
			    || meta_is_str_at(original, (current + 2), "T", "S", "")
			    || (
				(current == 0 || meta_is_str_at(original, (current - 1), "A", "O", "U", "E", ""))
				/** e.g., 'wachtler', 'wechsler', but not 'tichner' **/
				&& meta_is_str_at(original, (current + 2), "L", "R", "N", "M", "B", "H", "F", "V", "W", " ", "")
			       )
			   )
			    {
			    meta_add_str(primary, "K");
			    meta_add_str(secondary, "K");
			    }
			else
			    {
			    if (current > 0)
				{
				if (meta_is_str_at(original, 0, "MC", ""))
				    {
				    /* e.g., "McHugh" */
				    meta_add_str(primary, "K");
				    meta_add_str(secondary, "K");
				    }
				else
				    {
				    meta_add_str(primary, "X");
				    meta_add_str(secondary, "K");
				    }
				}
			    else
				{
				meta_add_str(primary, "X");
				meta_add_str(secondary, "X");
				}
			    }
			    current += 2;
			    break;
			}
		    
		    /** e.g, 'czerny' **/
		    if (meta_is_str_at(original, current, "CZ", "")
			&& !meta_is_str_at(original, (current - 2), "WICZ", ""))
			{
			meta_add_str(primary, "S");
			meta_add_str(secondary, "X");
			current += 2;
			break;
			}
		    
		    /** e.g., 'focaccia' **/
		    if (meta_is_str_at(original, (current + 1), "CIA", ""))
			{
			meta_add_str(primary, "X");
			meta_add_str(secondary, "X");
			current += 3;
			break;
			}
		    
		    /** Double 'C' rule. **/
		    if (
			meta_is_str_at(original, current, "CC", "")
			&& !(current == 1 && first_char == 'M') /* McClellan exception. */
		       )
			{
			/** 'bellocchio' but not 'bacchus' **/
			if (
			    meta_is_str_at(original, (current + 2), "I", "E", "H", "")
			    && !meta_is_str_at(original, (current + 2), "HU", "")
			   )
			    {
			    /** 'accident', 'accede' 'succeed' **/
			    if (
				(current == 1 && meta_get_char_at(original, current - 1) == 'A')
				|| meta_is_str_at(original, (current - 1), "UCCEE", "UCCES", "")
			       )
				{
				meta_add_str(primary, "KS");
				meta_add_str(secondary, "KS");
				/** 'bacci', 'bertucci', other italian **/
				}
			    else
				{
				meta_add_str(primary, "X");
				meta_add_str(secondary, "X");
				}
			    current += 3;
			    break;
			    }
			else
			    { /** Pierce's rule **/
			    meta_add_str(primary, "K");
			    meta_add_str(secondary, "K");
			    current += 2;
			    break;
			    }
			}
		    
		    if (meta_is_str_at(original, current, "CK", "CG", "CQ", ""))
			{
			meta_add_str(primary, "K");
			meta_add_str(secondary, "K");
			current += 2;
			break;
			}
		    
		    if (meta_is_str_at(original, current, "CI", "CE", "CY", ""))
			{
			/* Italian vs. English */
			if (meta_is_str_at(original, current, "CIO", "CIE", "CIA", ""))
			    {
			    meta_add_str(primary, "S");
			    meta_add_str(secondary, "X");
			    }
			else
			    {
			    meta_add_str(primary, "S");
			    meta_add_str(secondary, "S");
			    }
			current += 2;
			break;
			}
		    
		    /** else **/
		    meta_add_str(primary, "K");
		    meta_add_str(secondary, "K");
		    
		    /** Name sent in 'mac caffrey', 'mac gregor **/
		    if (meta_is_str_at(original, (current + 1), " C", " Q", " G", ""))
			current += 3;
		    else if (meta_is_str_at(original, (current + 1), "C", "K", "Q", "")
			     && !meta_is_str_at(original, (current + 1), "CE", "CI", ""))
			current += 2;
		    else
			current += 1;
		    break;
		    }
		
		case 'D':
		    {
		    if (meta_is_str_at(original, current, "DG", ""))
			{
			if (meta_is_str_at(original, (current + 2), "I", "E", "Y", ""))
			    {
			    /** e.g. 'edge' **/
			    meta_add_str(primary, "J");
			    meta_add_str(secondary, "J");
			    current += 3;
			    break;
			    }
			else
			    {
			    /** e.g. 'edgar' **/
			    meta_add_str(primary, "TK");
			    meta_add_str(secondary, "TK");
			    current += 2;
			    break;
			    }
			}
		    
		    if (meta_is_str_at(original, current, "DT", "DD", ""))
			{
			meta_add_str(primary, "T");
			meta_add_str(secondary, "T");
			current += 2;
			break;
			}
		    
		    /** else **/
		    meta_add_str(primary, "T");
		    meta_add_str(secondary, "T");
		    current += 1;
		    break;
		    }
		
		case 'F':
		    {
		    current += (next_char == 'F') ? 2 : 1;
		    meta_add_str(primary, "F");
		    meta_add_str(secondary, "F");
		    break;
		    }
		
		case 'G':
		    {
		    if (next_char == 'H')
			{
			/** 'Vghee' */
			if (current > 0 && !meta_is_vowel(original, (current - 1)))
			    {
			    meta_add_str(primary, "K");
			    meta_add_str(secondary, "K");
			    current += 2;
			    break;
			    }
			
			if (current < 3)
			    {
			    /** 'ghislane', 'ghiradelli' **/
			    if (current == 0)
				{
				if (meta_get_char_at(original, (current + 2)) == 'I')
				    {
				    meta_add_str(primary, "J");
				    meta_add_str(secondary, "J");
				    }
				else
				    {
				    meta_add_str(primary, "K");
				    meta_add_str(secondary, "K");
				    }
				current += 2;
				break;
				}
			    }
			
			if (
			    /** Parker's rule (with some further refinements) - e.g., 'hugh' **/
			    (current > 1 && meta_is_str_at(original, (current - 2), "B", "H", "D", ""))
			    /** e.g., 'bough' **/
			    || (current > 2 && meta_is_str_at(original, (current - 3), "B", "H", "D", ""))
			    /** e.g., 'broughton' **/
			    || (current > 3 && meta_is_str_at(original, (current - 4), "B", "H", ""))
			)
			    {
			    current += 2;
			    break;
			    }
			else
			    {
			    /** e.g., 'laugh', 'McLaughlin', 'cough', 'gough', 'rough', 'tough' **/
			    if (
				current > 2
				&& meta_get_char_at(original, (current - 1)) == 'U'
				&& meta_is_str_at(original, (current - 3), "C", "G", "L", "R", "T", "")
			       )
				{
				meta_add_str(primary, "F");
				meta_add_str(secondary, "F");
				}
			    else if (current > 0 && meta_get_char_at(original, (current - 1)) != 'I')
				{
				meta_add_str(primary, "K");
				meta_add_str(secondary, "K");
				}
			    
			    current += 2;
			    break;
			    }
			}
		    
		    if (next_char == 'N')
			{
			if (current == 1 && !is_slavo_germanic && meta_is_vowel(original, 0))
			    {
			    meta_add_str(primary, "KN");
			    meta_add_str(secondary, "N");
			    }
			else
			    /** not e.g. 'cagney' **/
			    if (
				next_char != 'Y'
				&& !is_slavo_germanic
				&& !meta_is_str_at(original, (current + 2), "EY", "")
			       )
				{
				meta_add_str(primary, "N");
				meta_add_str(secondary, "KN");
				}
			else
			    {
			    meta_add_str(primary, "KN");
			    meta_add_str(secondary, "KN");
			    }
			current += 2;
			break;
			}
		    
		    /** 'tagliaro' **/
		    if (
			!is_slavo_germanic
			&& meta_is_str_at(original, (current + 1), "LI", "")
		       )
			{
			meta_add_str(primary, "KL");
			meta_add_str(secondary, "L");
			current += 2;
			break;
			}
		    
		    /** -ges-,-gep-,-gel-, -gie- at beginning **/
		    if (
			current == 0
			&& (
			    next_char == 'Y'
			    || meta_is_str_at(
				original, (current + 1),
				"ES", "EP", "EB", "EL", "EY", "IB",
				"IL", "IN", "IE", "EI", "ER", ""
			    )
			   )
		       )
			{
			meta_add_str(primary, "K");
			meta_add_str(secondary, "J");
			current += 2;
			break;
			}
		    
		    /** -ger-,  -gy- **/
		    if (
			(next_char == 'Y' || meta_is_str_at(original, (current + 1), "ER", ""))
			/** Exceptions. **/
			&& !meta_is_str_at(original, 0, "DANGER", "RANGER", "MANGER", "")
			&& !meta_is_str_at(original, (current - 1), "E", "I", "RGY", "OGY", "")
		       )
			{
			meta_add_str(primary, "K");
			meta_add_str(secondary, "J");
			current += 2;
			break;
			}
		    
		    /** Italian e.g, 'biaggi' **/
		    if (
			meta_is_str_at(original, (current + 1), "E", "I", "Y", "")
			|| meta_is_str_at(original, (current - 1), "AGGI", "OGGI", "")
		       )
			{
			/** Obvious germanic. **/
			if (meta_is_str_at(original, 0, "SCH", "VAN ", "VON ", "")
			    || meta_is_str_at(original, (current + 1), "ET", ""))
			    {
			    meta_add_str(primary, "K");
			    meta_add_str(secondary, "K");
			    }
			else
			    {
			    /** Always soft, if french ending. **/
			    if (meta_is_str_at(original, (current + 1), "IER ", ""))
				{
				meta_add_str(primary, "J");
				meta_add_str(secondary, "J");
				}
			    else
				{
				meta_add_str(primary, "J");
				meta_add_str(secondary, "K");
				}
			    }
			current += 2;
			break;
		    }
		    
		    current += (next_char == 'G') ? 2 : 1;
		    meta_add_str(primary, "K");
		    meta_add_str(secondary, "K");
		    break;
		    }
		
		case 'H':
		    {
		    /** Only keep if first & before vowel or between 2 vowels. **/
		    if (
			(current == 0 || meta_is_vowel(original, (current - 1)))
			&& meta_is_vowel(original, current + 1)
		       )
			{
			meta_add_str(primary, "H");
			meta_add_str(secondary, "H");
			current += 2;
			}
		    else /* also takes care of 'HH' */
			current += 1;
		    break;
		    }
		
		case 'J':
		    {
		    /** Obvious spanish, 'jose', 'san jacinto' **/
		    const bool has_jose_next = meta_is_str_at(original, current, "JOSE", "");
		    const bool starts_with_san = meta_is_str_at(original, 0, "SAN ", "");
		    if (has_jose_next || starts_with_san)
			{
			if (
			    starts_with_san
			    /** I don't know what this condition means. **/
			    || (current == 0 && meta_get_char_at(original, current + 4) == ' ')
			   )
			    {
			    meta_add_str(primary, "H");
			    meta_add_str(secondary, "H");
			    }
			else
			    {
			    meta_add_str(primary, "J");
			    meta_add_str(secondary, "H");
			    }
			current += 1;
			break;
			}
		    
		    if (current == 0 && !has_jose_next)
			{
			meta_add_str(primary, "J"); /* Yankelovich/Jankelowicz */
			meta_add_str(secondary, "A");
			}
		    else
			{
			/** spanish pron. of e.g. 'bajador' **/
			if (
			    !is_slavo_germanic
			    && (next_char == 'A' || next_char == 'O')
			    && meta_is_vowel(original, (current - 1))
			   )
			    {
			    meta_add_str(primary, "J");
			    meta_add_str(secondary, "H");
			    }
			else
			    {
			    if (current == last)
				{
				meta_add_str(primary, "J");
				meta_add_str(secondary, "");
				}
			    else
				{
				if (
				    !meta_is_str_at(original, (current + 1), "L", "T", "K", "S", "N", "M", "B", "Z", "")
				    && !meta_is_str_at(original, (current - 1), "S", "K", "L", "")
				   )
				    {
				    meta_add_str(primary, "J");
				    meta_add_str(secondary, "J");
				    }
				}
			    }
			}
		    
		    current += (next_char == 'J') ? 2 : 1;
		    break;
		    }
		
		case 'K':
		    {
		    current += (next_char == 'K') ? 2 : 1;
		    meta_add_str(primary, "K");
		    meta_add_str(secondary, "K");
		    break;
		    }
		
		case 'L':
		    {
		    if (next_char == 'L')
			{
			/** Spanish e.g. 'cabrillo', 'gallegos' **/
			if (
			    (
			     current == length - 3
			     && meta_is_str_at(original, (current - 1), "ILLO", "ILLA", "ALLE", "")
			    )
			    || (
				meta_is_str_at(original, (current - 1), "ALLE", "")
				&& (
				    meta_is_str_at(original, (last - 1), "AS", "OS", "")
				    || meta_is_str_at(original, last, "A", "O", "")
				   )
			       )
			   )
			    {
			    meta_add_str(primary, "L");
			    meta_add_str(secondary, "");
			    current += 2;
			    break;
			    }
			current += 2;
			}
		    else
			current += 1;
		    meta_add_str(primary, "L");
		    meta_add_str(secondary, "L");
		    break;
		    }
		
		case 'M':
		    {
		    current += (
			(
			 meta_is_str_at(original, (current - 1), "UMB", "")
			 && (current + 1 == last || meta_is_str_at(original, (current + 2), "ER", ""))
			)
			/** 'dumb','thumb' **/
			|| next_char == 'M'
		    ) ? 2 : 1;
		    meta_add_str(primary, "M");
		    meta_add_str(secondary, "M");
		    break;
		    }
		
		case 'N':
		    {
		    current += (next_char == 'N') ? 2 : 1;
		    meta_add_str(primary, "N");
		    meta_add_str(secondary, "N");
		    break;
		    }
		
		case 'P':
		    {
		    if (next_char == 'H')
			{
			meta_add_str(primary, "F");
			meta_add_str(secondary, "F");
			current += 2;
			break;
			}
		    
		    /** Also account for "campbell", "raspberry" **/
		    current += (meta_is_str_at(original, (current + 1), "P", "B", "")) ? 2 : 1;
		    meta_add_str(primary, "P");
		    meta_add_str(secondary, "P");
		    break;
		    }
		
		case 'Q':
		    {
		    current += (next_char == 'Q') ? 2 : 1;
		    meta_add_str(primary, "K");
		    meta_add_str(secondary, "K");
		    break;
		    }
		
		case 'R':
		    {
		    /** French e.g. 'rogier', but exclude 'hochmeier' **/
		    const bool no_primary = (
			!is_slavo_germanic
			&& current == last
			&& meta_is_str_at(original, (current - 2), "IE", "")
			&& !meta_is_str_at(original, (current - 4), "ME", "MA", "")
		    );
		    
		    meta_add_str(primary, (no_primary) ? "" : "R");
		    meta_add_str(secondary, "R");
		    current += (next_char == 'R') ? 2 : 1;
		    break;
		}
		
		case 'S':
		    {
		    /** Special cases 'island', 'isle', 'carlisle', 'carlysle' **/
		    if (meta_is_str_at(original, (current - 1), "ISL", "YSL", ""))
			{
			current += 1;
			break;
			}
		    
		    /** Special case 'sugar-' **/
		    if (current == 0 && meta_is_str_at(original, current, "SUGAR", ""))
			{
			meta_add_str(primary, "X");
			meta_add_str(secondary, "S");
			current += 1;
			break;
			}
		    
		    if (meta_is_str_at(original, current, "SH", ""))
			{
			const bool germanic = meta_is_str_at(original, (current + 1), "HEIM", "HOEK", "HOLM", "HOLZ", "");
			const char* sound = (germanic) ? "S" : "X";
			meta_add_str(primary, sound);
			meta_add_str(secondary, sound);
			current += 2;
			break;
			}
		    
		    /** Italian & Armenian. **/
		    if (meta_is_str_at(original, current, "SIO", "SIA", "SIAN", ""))
			{
			meta_add_str(primary, "S");
			meta_add_str(secondary, (is_slavo_germanic) ? "S" : "X");
			current += 3;
			break;
			}
		    
		    /** german & anglicisations, e.g. 'smith' match 'schmidt', 'snider' match 'schneider' **/
		    /** also, -sz- in slavic language although in hungarian it is pronounced 's' **/
		    if (current == 0 && meta_is_str_at(original, (current + 1), "M", "N", "L", "W", ""))
			{
			meta_add_str(primary, "S");
			meta_add_str(secondary, "X");
			current += 1;
			break;
			}
		    if (meta_is_str_at(original, (current + 1), "Z", ""))
			{
			meta_add_str(primary, "S");
			meta_add_str(secondary, "X");
			current += 2;
			break;
			}
		    
		    if (meta_is_str_at(original, current, "SC", ""))
			{
			/** Schlesinger's rule. **/
			if (meta_get_char_at(original, current + 2) == 'H')
			    {
			    /** Dutch origin, e.g. 'school', 'schooner' **/
			    if (meta_is_str_at(original, (current + 3), "OO", "ER", "EN", "UY", "ED", "EM", ""))
				{
				/** 'schermerhorn', 'schenker' **/
				const bool x_sound = meta_is_str_at(original, (current + 3), "ER", "EN", "");
				meta_add_str(primary, (x_sound) ? "X" : "SK");
				meta_add_str(secondary, "SK");
				current += 3;
				break;
				}
			    else
				{
				const bool s_sound = (
				    current == 0
				    && !meta_is_vowel(original, 3)
				    && meta_get_char_at(original, 3) != 'W'
				);
				meta_add_str(primary, "X");
				meta_add_str(secondary, (s_sound) ? "S" : "X");
				current += 3;
				break;
				}
			    }
			
			/** Default case. **/
			const char* sound = (meta_is_str_at(original, (current + 2), "E", "I", "Y", "")) ? "S" : "SK";
			meta_add_str(primary, sound);
			meta_add_str(secondary, sound);
			current += 3;
			break;
			}
		    
		    /** French e.g. 'resnais', 'artois' **/
		    const bool no_primary = (current == last && meta_is_str_at(original, (current - 2), "AI", "OI", ""));
		    meta_add_str(primary, (no_primary) ? "" : "S");
		    meta_add_str(secondary, "S");
		    current += (meta_is_str_at(original, (current + 1), "S", "Z", "")) ? 2 : 1;
		    break;
		    }
		
		case 'T':
		    {
		    if (meta_is_str_at(original, current, "TIA", "TCH", "TION", ""))
			{
			meta_add_str(primary, "X");
			meta_add_str(secondary, "X");
			current += 3;
			break;
			}
		    
		    if (meta_is_str_at(original, current, "TH", "TTH", ""))
			{
			/** Special case 'thomas', 'thames' or germanic. **/
			if (
			    meta_is_str_at(original, (current + 2), "OM", "AM", "")
			    || meta_is_str_at(original, 0, "SCH", "VAN ", "VON ", "")
			   )
			    meta_add_str(primary, "T");
			else
			    meta_add_str(primary, "0"); /* Yes, zero. */
			meta_add_str(secondary, "T");
			current += 2;
			break;
			}
		    
		    meta_add_str(primary, "T");
		    meta_add_str(secondary, "T");
		    current += (meta_is_str_at(original, (current + 1), "T", "D", "")) ? 2 : 1;
		    break;
		    }
		
		case 'V':
		    {
		    meta_add_str(primary, "F");
		    meta_add_str(secondary, "F");
		    current += (next_char == 'V') ? 2 : 1;
		    break;
		    }
		
		case 'W':
		    {
		    /** Can also be in middle of word. **/
		    if (meta_is_str_at(original, current, "WR", ""))
			{
			meta_add_str(primary, "R");
			meta_add_str(secondary, "R");
			current += 2;
			break;
			}
		    
		    const bool next_is_vowel = meta_is_vowel(original, current + 1);
		    if (current == 0 && (next_is_vowel || meta_is_str_at(original, current, "WH", "")))
			{
			/** Wasserman should match Vasserman. **/
			meta_add_str(primary, "A");
			meta_add_str(secondary, (next_is_vowel) ? "F" : "A");
			}
		    
		    /** Arnow should match Arnoff. **/
		    if ((current == last && meta_is_vowel(original, current - 1))
			|| meta_is_str_at(original, (current - 1), "EWSKI", "EWSKY", "OWSKI", "OWSKY", "")
			|| meta_is_str_at(original, 0, "SCH", "")
		       )
			{
			meta_add_str(primary, "");
			meta_add_str(secondary, "F");
			current += 1;
			break;
			}
		    
		    /** Polish e.g. 'filipowicz' **/
		    if (meta_is_str_at(original, current, "WICZ", "WITZ", ""))
			{
			meta_add_str(primary, "TS");
			meta_add_str(secondary, "FX");
			current += 4;
			break;
			}
		    
		    /** Else skip it. **/
		    current += 1;
		    break;
		    }
		
		case 'X':
		    {
		    /** French e.g. breaux **/
		    const bool silent = (
			current == last
			&& (
			    meta_is_str_at(original, (current - 2), "AU", "OU", "")
			    || meta_is_str_at(original, (current - 3), "IAU", "EAU", "")
			)
		    );
		    if (!silent)
			{
			meta_add_str(primary, "KS");
			meta_add_str(secondary, "KS");
			}
		    
		    current += (meta_is_str_at(original, (current + 1), "C", "X", "")) ? 2 : 1;
		    break;
		    }
		
		case 'Z':
		    {
		    /** Chinese pinyin e.g. 'zhao' **/
		    if (next_char == 'H')
			{
			meta_add_str(primary, "J");
			meta_add_str(secondary, "J");
			current += 2;
			break;
			}
		    
		    const bool has_t_sound = (
			meta_is_str_at(original, (current + 1), "ZO", "ZI", "ZA", "")
			|| (is_slavo_germanic && current > 0 && meta_get_char_at(original, (current - 1)) != 'T')
		    );
		    meta_add_str(primary, "S");
		    meta_add_str(secondary, (has_t_sound) ? "TS" : "S");
		    current += (next_char == 'Z') ? 2 : 1;
		    break;
		    }
		
		default:
		    current += 1;
		}
	    }
	
	*primary_code = primary->str;
	*secondary_code = secondary->str;
	
	meta_destroy_string(original);
	meta_destroy_string(primary);
	meta_destroy_string(secondary);
    
    return;
    }

#ifdef TESTING
/*** Built in test cases, written by Israel with inspiration from comments in
 *** the above code, test cases written by Maurice Aubrey, and some words
 *** suggested by AI.
 *** 
 *** These tests have been integrated into the Centrallix testing environment,
 *** where they can be run using `export TONLY=exp_fn_double_metaphone_00`,
 *** followed by make test, in the Centrallix directory.
 *** 
 *** The can also be run here by executing the following commands in the
 *** centrallix/expression directory, which aditionally generates a coverage
 *** report. These tests cover all parts of the double metaphone algorithm,
 *** although some of the error cases in various helper functions (such as
 *** meta_destroy_string(null)) are not covered by testing.
 *** 
 *** Commands:
 *** gcc exp_double_metaphone.c -o exp_double_metaphone.o -I .. -DTESTING -fprofile-arcs -ftest-coverage -O0
 *** ./exp_double_metaphone.o
 *** gcov exp_double_metaphone.c
 ***/

unsigned int num_tests_passed = 0u, num_tests_failed = 0u;

void
test(const char* input, const char* expected_primary, const char* expected_secondary)
    {
    char* codes[2];
	
	/** Run DoubleMetaphone() and extract results. **/
	char* actual_primary;
	char* actual_secondary;
	meta_double_metaphone(
	    input,
	    memset(&actual_primary, 0, sizeof(actual_primary)),
	    memset(&actual_secondary, 0, sizeof(actual_secondary))
	);
	
	/** Test for correct value. **/
	if (!strcmp(expected_primary, actual_primary) &&
	    !strcmp(expected_secondary, actual_secondary))
	    num_tests_passed++;
	else
	    {
	    printf(
		"\nTEST FAILED: \"%s\"\n"
		"Expected: %s %s\n"
		"Actual: %s %s\n",
		input,
		expected_primary, expected_secondary,
		actual_primary, actual_secondary
	    );
	    num_tests_failed++;
	    }
    
    return;
    }

// Special thanks to the following websites for double checking the correct results:
// 1: https://words.github.io/double-metaphone
// 2: https://mainegenealogy.net/metaphone_converter.asp
// 3: https://en.toolpage.org/tool/metaphone
void
run_tests(void)
    {
	printf("\nRunning tests...\n");
	
	/** Test that always fails. **/
	// test("This", "test", "fails.");
	
	/** Invalid string tests, by Israel. **/
	fprintf(stderr, "Expect two warnings between these two lines:\n");
	fprintf(stderr, "----------------\n");
	test(NULL, "", "");
	test("", "", "");
	fprintf(stderr, "----------------\n");
	
	/** Basic tests, by Israel. **/
	test("Test", "TST", "TST");
	test("Basic", "PSK", "PSK");
	test("Centrallix", "SNTRLKS", "SNTRLKS");
	test("Lawrence", "LRNS", "LRNS");
	test("Philips", "FLPS", "FLPS");
	test("Acceptingness", "AKSPTNNS", "AKSPTNKNS");
	test("Supercalifragilisticexpialidocious", "SPRKLFRJLSTSKSPLTSS", "SPRKLFRKLSTSKSPLTXS");
	test("Suoicodilaipxecitsiligarfilacrepus", "SKTLPKSSTSLKRFLKRPS", "SKTLPKSSTSLKRFLKRPS");	
	
	/** Match tests, from code comments above. **/
	test("Smith", "SM0", "XMT");
	test("Schmidt", "XMT", "SMT");
	test("Snider", "SNTR", "XNTR");
	test("Schneider", "XNTR", "SNTR");
	test("Arnow", "ARN", "ARNF");
	test("Arnoff", "ARNF", "ARNF");
	
	/** Example tests, from examples in code comments above. **/
	test("Accede", "AKST", "AKST");
	test("Accident", "AKSTNT", "AKSTNT");
	test("Actually", "AKTL", "AKTL");
	test("Arch", "ARX", "ARK");
	test("Artois", "ART", "ARTS");
	test("Bacchus", "PKS", "PKS");
	test("Bacci", "PX", "PX");
	test("Bajador", "PJTR", "PHTR");
	test("Bellocchio", "PLX", "PLX");
	test("Bertucci", "PRTX", "PRTX");
	test("Biaggi", "PJ", "PK");
	test("Bough", "P", "P");
	test("Breaux", "PR", "PR");
	test("Broughton", "PRTN", "PRTN");
	test("Cabrillo", "KPRL", "KPR");
	test("Caesar", "SSR", "SSR");
	test("Cagney", "KKN", "KKN");
	test("Campbell", "KMPL", "KMPL");
	test("Carlisle", "KRLL", "KRLL");
	test("Carlysle", "KRLL", "KRLL");
	test("Chemistry", "KMSTR", "KMSTR");
	test("Chianti", "KNT", "KNT");
	test("Chorus", "KRS", "KRS");
	test("Cough", "KF", "KF");
	test("Czerny", "SRN", "XRN");
	test("Dumb", "TM", "TM");
	test("Edgar", "ATKR", "ATKR");
	test("Edge", "AJ", "AJ");
	test("Filipowicz", "FLPTS", "FLPFX");
	test("Focaccia", "FKX", "FKX");
	test("Gallegos", "KLKS", "KKS");
	test("Germanic", "KRMNK", "JRMNK");
	test("Ghiradelli", "JRTL", "JRTL");
	test("Ghislane", "JLN", "JLN");
	test("Gospel", "KSPL", "KSPL");
	test("Gough", "KF", "KF");
	test("Greek", "KRK", "KRK");
	test("Hochmeier", "HKMR", "HKMR");
	test("Hugh", "H", "H");
	test("Island", "ALNT", "ALNT");
	test("Isle", "AL", "AL");
	test("Italian", "ATLN", "ATLN");
	test("Jankelowicz", "JNKLTS", "ANKLFX");
	test("Jose", "HS", "HS");
	test("Laugh", "LF", "LF");
	test("Mac Caffrey", "MKFR", "MKFR");
	test("Mac Gregor", "MKRKR", "MKRKR");
	test("Manager", "MNKR", "MNJR");
	test("McHugh", "MK", "MK");
	test("McLaughlin", "MKLFLN", "MKLFLN");
	test("Michael", "MKL", "MXL");
	test("Middle", "MTL", "MTL");
	test("Orchestra", "ARKSTR", "ARKSTR");
	test("Orchid", "ARKT", "ARKT");
	test("Pinyin", "PNN", "PNN");
	test("Raspberry", "RSPR", "RSPR");
	test("Resnais", "RSN", "RSNS");
	test("Rogier", "RJ", "RJR");
	test("Rough", "RF", "RF");
	test("Salvador", "SLFTR", "SLFTR");
	test("San jacinto", "SNHSNT", "SNHSNT");
	test("Schenker", "XNKR", "SKNKR");
	test("Schermerhorn", "XRMRRN", "SKRMRRN");
	test("Schlesinger", "XLSNKR", "SLSNJR");
	test("School", "SKL", "SKL");
	test("Schooner", "SKNR", "SKNR");
	test("Succeed", "SKST", "SKST");
	test("Sugar", "XKR", "SKR");
	test("Sugary", "XKR", "SKR");
	test("Tagliaro", "TKLR", "TLR");
	test("Thames", "TMS", "TMS");
	test("Thomas", "TMS", "TMS");
	test("Thumb", "0M", "TM");
	test("Tichner", "TXNR", "TKNR");
	test("Tough", "TF", "TF");
	test("Vghee", "FK", "FK");
	test("Wachtler", "AKTLR", "FKTLR");
	test("Wechsler", "AKSLR", "FKSLR");
	test("Word", "ART", "FRT");
	test("Xavier", "SF", "SFR");
	test("Yankelovich", "ANKLFX", "ANKLFK");
	test("Zhao", "J", "J");
	
	/** Interesting Edge Case: "McClellan" **/
	/*** Note: Sources (1) and (3) both include a double K ("MKKLLN"), but the
	 *** original code on GitHub and mainegenealogy.net do not. I chose "MKLLN"
	 *** to be correct because I personally do not pronounce the second c.
	 ***/
	test("McClellan", "MKLLN", "MKLLN");
	
	/** Maurice Aubrey's Tests. **/
	/** Source: https://github.com/gitpan/Text-DoubleMetaphone/blob/master/t/words.txt **/
	test("maurice", "MRS", "MRS");
	test("aubrey", "APR", "APR");
	test("cambrillo", "KMPRL", "KMPR");
	test("heidi", "HT", "HT");
	test("katherine", "K0RN", "KTRN");
	test("catherine", "K0RN", "KTRN");
	test("richard", "RXRT", "RKRT");
	test("bob", "PP", "PP");
	test("eric", "ARK", "ARK");
	test("geoff", "JF", "KF");
	test("dave", "TF", "TF");
	test("ray", "R", "R");
	test("steven", "STFN", "STFN");
	test("bryce", "PRS", "PRS");
	test("randy", "RNT", "RNT");
	test("bryan", "PRN", "PRN");
	test("brian", "PRN", "PRN");
	test("otto", "AT", "AT");
	test("auto", "AT", "AT");
	
	/** GPT-5 Coverage Tests. **/
	/*** GPT-5 mini (Preview) running in GitHub Copilot suggested the words
	 *** after analizing a generated coverage report, and I (Israel) used
	 *** them to write the tests below.  I kept the AI's reasoning for tests,
	 *** while removing tests that did not contribute any coverage, but after
	 *** a few reprompts, the AI started just giving words without reasoning.
	 *** I guess we were both getting pretty tired of writing tests.
	 ***/
	test("Abbott", "APT", "APT"); /* double-B ("BB") handling. */
	test("Back", "PK", "PK"); /* "CK"/"CG"/"CQ" branch. */
	test("Bacher", "PKR", "PKR"); /* matches "...BACHER" / ACH special-case. */
	test("Charles", "XRLS", "XRLS"); /* initial "CH" -> the branch that maps to "X"/"X" at start. */
	test("Ghana", "KN", "KN"); /* initial "GH" special-start handling. */
	test("Gnome", "NM", "NM"); /* "GN" sequence handling. */
	test("Raj", "RJ", "R"); /* J at end (exercise J-last behavior). */
	test("Quentin", "KNTN", "KNTN"); /* Q case (Q -> K mapping). */
	test("Who", "A", "A"); /* "WH" at start handling. */
	test("Shoemaker", "XMKR", "XMKR"); /* "SH" general mapping paths. */
	test("Sian", "SN", "XN"); /* "SIO"/"SIA"/"SIAN" branch. */
	test("Scold", "SKLT", "SKLT"); /* "SC" default / "SK" vs other SC subcases. */
	test("Station", "STXN", "STXN"); /* "TION" -> X mapping. */
	test("Match", "MX", "MX"); /* "TCH"/"TIA" -> X mapping. */
	test("Pizza", "PS", "PTS"); /* double-Z ("ZZ") handling. */
	test("Agnes", "AKNS", "ANS"); /* "GN" at index 1 (GN handling that yields KN / N). */
	test("Science", "SNS", "SNS"); /* "SC" followed by I (SC + I/E/Y branch). */
	test("Van Gogh", "FNKK", "FNKK");
	test("Josef", "JSF", "HSF");
	test("Object", "APJKT", "APJKT");
	test("Sholz", "SLS", "SLS");
	test("Scharf", "XRF", "XRF");
	test("Kasia", "KS", "KS");
	test("Van Geller", "FNKLR", "FNKLR");
	
	const unsigned int total_tests = num_tests_passed + num_tests_failed;
	printf("\nTests completed!\n");
	printf("    > Failed: %u\n", num_tests_failed);
	printf("    > Skipped: %u\n", 0u); /* Implementation removed. */
	printf("    > Passed: %u/%u\n", num_tests_passed, total_tests);
    
    return;
    }

int main(void)
    {
	run_tests();
    
    return 0;
    }

/** Prevent scope leak. **/
#undef META_FREE
#undef META_MALLOC
#undef META_REALLOC
#undef SAFE_MALLOC
#undef SAFE_REALLOC

#endif
