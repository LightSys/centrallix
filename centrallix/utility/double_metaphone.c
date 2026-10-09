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
/* 		written by Philips'. This implementation was written by	*/
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
 *** Original Source: https://github.com/gitpan/Text-DoubleMetaphone
 ***/

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "cxlib/newmalloc.h"
#include "cxlib/strtcpy.h"
#include "cxlib/util.h"
#include "cxlib/expect.h"

typedef struct
    {
    char* str;
    size_t length;
    size_t bufsize;
    int free_str_on_destroy;
    }
    MetaString, *pMetaString;

/*** Allocates a new MetaString.
 *** 
 *** @param init_str The initial size of the string.
 *** @returns The new MetaString, or NULL if an error occurs.
 ***/
pMetaString
meta_new_string(const char* init_str)
    {
    pMetaString s;
    char empty_string[] = "";
    
	s = (pMetaString)check_ptr(nmSysMalloc(sizeof(MetaString)));
	if (UNLIKELY(s == NULL)) goto err_free;
	
	if (init_str == NULL)
	    init_str = empty_string;
	
	s->length = strlen(init_str);
	/** Preallocate a bit more for potential growth. **/
	s->bufsize = s->length + 7u;
	
	s->str = (char*)check_ptr(nmSysMalloc(s->bufsize * sizeof(char)));
	if (UNLIKELY(s->str == NULL)) goto err_free;
	
	strtcpy(s->str, init_str, s->bufsize);
	s->free_str_on_destroy = 1;
    
	return s;
	
    err_free:
	if (s != NULL)
	    {
	    if (s->str != NULL) nmSysFree(s->str);
	    nmSysFree(s);
	    }
	
	return NULL;
    }

/*** Frees a MetaString.
 *** 
 *** @param s The MetaString.
 ***/
void
meta_destroy_string(pMetaString s)
    {
	if (UNLIKELY(s == NULL))
	    return;
	
	if (s->free_str_on_destroy && s->str != NULL)
	    nmSysFree(s->str);
	
	nmSysFree(s);
    
    return;
    }

/*** Increases a MetaString's buffer size.
 *** 
 *** @param s The pMetaString being modified.
 *** @param chars_needed Minimum number of characters to increase buffer size.
 *** @returns 0 if successful, or -1 if an error occurs.
 ***/
int
meta_increase_buffer(pMetaString s, const size_t chars_needed)
    {
	s->bufsize += chars_needed + 8u;
	s->str = check_ptr(nmSysRealloc(s->str, s->bufsize * sizeof(char)));
	if (UNLIKELY(s->str == NULL)) return -1;
    
    return 0;
    }

/*** Convert all characters of a MetaString to uppercase.
 *** 
 *** @param s The MetaString being modified.
 ***/
void
meta_make_upper(pMetaString s)
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
meta_is_out_of_bounds(pMetaString s, unsigned int pos)
    {
    return (s->length <= pos);
    }

/*** Checks if a character in a MetaString is a vowel.
 *** 
 *** @param s The MetaString being checked.
 *** @param pos The character location to check within the MetaString.
 ***/
bool
meta_is_vowel(pMetaString s, unsigned int pos)
    {
	if (UNLIKELY(meta_is_out_of_bounds(s, pos))) return 0;
	
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
meta_is_slavo_germanic(pMetaString s)
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
meta_get_char_at(pMetaString s, unsigned int pos)
    {
    return (UNLIKELY(meta_is_out_of_bounds(s, pos))) ? '\0' : ((char) *(s->str + pos));
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
meta_is_str_at(pMetaString s, unsigned int start, ...)
    {
    va_list ap;
    bool found = false;
    
	/** Should never happen. **/
	if (UNLIKELY(meta_is_out_of_bounds(s, start)))
	    return false;
	
	const char* pos = (s->str + start);
	va_start(ap, start);
	
	char* test;
	do
	    {
	    test = va_arg(ap, char*);
	    if (test[0] != '\0' && (strncmp(pos, test, strlen(test)) == 0))
		{
		found = true;
		break;
		}
	    }
	while (test[0] != '\0');
	
	va_end(ap);
    
    return found;
    }

/*** Adds a string to a MetaString, expanding the MetaString if needed.
 *** 
 *** @param s The MetaString being modified.
 *** @param new_str The string being added.
 *** @returns 0 if successful, or -1 if an error occurs.
 ***/
int
meta_add_str(pMetaString s, const char* new_str)
    {
	if (UNLIKELY(new_str == NULL))
	    return -1;
	
	/** Increase the buffer to the required size. **/
	const size_t add_length = strlen(new_str);
	const size_t new_length = s->length + add_length + 1;
	if (UNLIKELY(new_length > s->bufsize) && check(meta_increase_buffer(s, add_length)) != 0)
	    return -1;
	
	/** Write the data to the buffer. **/
	strtcat(s->str, new_str, s->bufsize);
	s->length += add_length;
    
    return 0;
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
 *** @returns 0 if successful, or -1 if an error occurs.
 ***/
int
meta_double_metaphone(const char* str, char** primary_code, char** secondary_code)
    {
    int ret = -1;
    
	/** Edge cases. **/
	if (UNLIKELY(str == NULL))
	    {
	    fprintf(stderr, "Error: Missing input string.\n");
	    goto end_free;
	    }
	const size_t length = strlen(str);
	if (UNLIKELY(length == 0lu))
	    {
	    fprintf(stderr, "Error: Empty input string.\n");
	    goto end_free;
	    }
	if (UNLIKELY(primary_code == NULL))
	    {
	    fprintf(stderr, "Error: Missing a pointer to store primary code.\n");
	    goto end_free;
	    }
	if (UNLIKELY(secondary_code == NULL))
	    {
	    fprintf(stderr, "Error: Missing a pointer to store secondary code.\n");
	    goto end_free;
	    }
	
	/** Declare iteration variables. **/
	unsigned int current = 0;
	const unsigned int last = (unsigned int)(length - 1);
	
	/** Pad original so we can index beyond end. **/
	pMetaString original = check_ptr(meta_new_string(str));
	if (UNLIKELY(original == NULL)) goto end_free;
	meta_make_upper(original);
	if (check(meta_add_str(original, "     ")) != 0) goto end_free;
	
	/** Allocate the primary and secondary output strings. **/
	pMetaString primary = check_ptr(meta_new_string(""));
	pMetaString secondary = check_ptr(meta_new_string(""));
	if (UNLIKELY(primary == NULL || secondary == NULL)) goto end_free;
	
	/** Skip these if they are at start of a word. **/
	if (meta_is_str_at(original, 0, "GN", "KN", "PN", "WR", "PS", ""))
	    current += 1;
	
	/** Initial 'X' is pronounced 'Z' e.g. 'Xavier' **/
	const char first_char = meta_get_char_at(original, 0);
	if (first_char == 'X')
	    {
	    if (check(meta_add_str(primary, "S")) != 0) goto end_free; /* 'Z' maps to 'S' */
	    if (check(meta_add_str(secondary, "S")) != 0) goto end_free;
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
			if (check(meta_add_str(primary, "A") != 0)) goto end_free;
			if (check(meta_add_str(secondary, "A") != 0)) goto end_free;
			}
		    current += 1;
		    break;	
		    }
		
		case 'B':
		    {
		    /** "-mb", e.g", "dumb", already skipped over... **/
		    if (check(meta_add_str(primary, "P") != 0)) goto end_free;
		    if (check(meta_add_str(secondary, "P") != 0)) goto end_free;
		    
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
			if (check(meta_add_str(primary, "K") != 0)) goto end_free;
			if (check(meta_add_str(secondary, "K") != 0)) goto end_free;
			current += 2;
			break;
			}
		    
		    /** Special case 'caesar' **/
		    if (current == 0 && meta_is_str_at(original, current, "CAESAR", ""))
			{
			if (check(meta_add_str(primary, "S") != 0)) goto end_free;
			if (check(meta_add_str(secondary, "S") != 0)) goto end_free;
			current += 2;
			break;
			}
		    
		    /** Italian 'chianti' **/
		    if (meta_is_str_at(original, current, "CHIA", ""))
			{
			if (check(meta_add_str(primary, "K") != 0)) goto end_free;
			if (check(meta_add_str(secondary, "K") != 0)) goto end_free;
			current += 2;
			break;
			}
		    
		    if (meta_is_str_at(original, current, "CH", ""))
			{
			/** Find 'michael' **/
			if (current > 0 && meta_is_str_at(original, current, "CHAE", ""))
			    {
			    if (check(meta_add_str(primary, "K") != 0)) goto end_free;
			    if (check(meta_add_str(secondary, "X") != 0)) goto end_free;
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
			    if (check(meta_add_str(primary, "K") != 0)) goto end_free;
			    if (check(meta_add_str(secondary, "K") != 0)) goto end_free;
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
			    if (check(meta_add_str(primary, "K") != 0)) goto end_free;
			    if (check(meta_add_str(secondary, "K") != 0)) goto end_free;
			    }
			else
			    {
			    if (current > 0)
				{
				if (meta_is_str_at(original, 0, "MC", ""))
				    {
				    /* e.g., "McHugh" */
				    if (check(meta_add_str(primary, "K") != 0)) goto end_free;
				    if (check(meta_add_str(secondary, "K") != 0)) goto end_free;
				    }
				else
				    {
				    if (check(meta_add_str(primary, "X") != 0)) goto end_free;
				    if (check(meta_add_str(secondary, "K") != 0)) goto end_free;
				    }
				}
			    else
				{
				if (check(meta_add_str(primary, "X") != 0)) goto end_free;
				if (check(meta_add_str(secondary, "X") != 0)) goto end_free;
				}
			    }
			    current += 2;
			    break;
			}
		    
		    /** e.g, 'czerny' **/
		    if (meta_is_str_at(original, current, "CZ", "")
			&& !meta_is_str_at(original, (current - 2), "WICZ", ""))
			{
			if (check(meta_add_str(primary, "S") != 0)) goto end_free;
			if (check(meta_add_str(secondary, "X") != 0)) goto end_free;
			current += 2;
			break;
			}
		    
		    /** e.g., 'focaccia' **/
		    if (meta_is_str_at(original, (current + 1), "CIA", ""))
			{
			if (check(meta_add_str(primary, "X") != 0)) goto end_free;
			if (check(meta_add_str(secondary, "X") != 0)) goto end_free;
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
				if (check(meta_add_str(primary, "KS")) != 0) goto end_free;
				if (check(meta_add_str(secondary, "KS")) != 0) goto end_free;
				/** 'bacci', 'bertucci', other italian **/
				}
			    else
				{
				if (check(meta_add_str(primary, "X")) != 0) goto end_free;
				if (check(meta_add_str(secondary, "X")) != 0) goto end_free;
				}
			    current += 3;
			    break;
			    }
			else
			    { /** Pierce's rule **/
			    if (check(meta_add_str(primary, "K")) != 0) goto end_free;
			    if (check(meta_add_str(secondary, "K")) != 0) goto end_free;
			    current += 2;
			    break;
			    }
			}
		    
		    if (meta_is_str_at(original, current, "CK", "CG", "CQ", ""))
			{
			if (check(meta_add_str(primary, "K")) != 0) goto end_free;
			if (check(meta_add_str(secondary, "K")) != 0) goto end_free;
			current += 2;
			break;
			}
		    
		    if (meta_is_str_at(original, current, "CI", "CE", "CY", ""))
			{
			/* Italian vs. English */
			if (meta_is_str_at(original, current, "CIO", "CIE", "CIA", ""))
			    {
			    if (check(meta_add_str(primary, "S")) != 0) goto end_free;
			    if (check(meta_add_str(secondary, "X")) != 0) goto end_free;
			    }
			else
			    {
			    if (check(meta_add_str(primary, "S")) != 0) goto end_free;
			    if (check(meta_add_str(secondary, "S")) != 0) goto end_free;
			    }
			current += 2;
			break;
			}
		    
		    /** else **/
		    if (check(meta_add_str(primary, "K")) != 0) goto end_free;
		    if (check(meta_add_str(secondary, "K")) != 0) goto end_free;
		    
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
			    if (check(meta_add_str(primary, "J")) != 0) goto end_free;
			    if (check(meta_add_str(secondary, "J")) != 0) goto end_free;
			    current += 3;
			    break;
			    }
			else
			    {
			    /** e.g. 'edgar' **/
			    if (check(meta_add_str(primary, "TK")) != 0) goto end_free;
			    if (check(meta_add_str(secondary, "TK")) != 0) goto end_free;
			    current += 2;
			    break;
			    }
			}
		    
		    if (meta_is_str_at(original, current, "DT", "DD", ""))
			{
			if (check(meta_add_str(primary, "T")) != 0) goto end_free;
			if (check(meta_add_str(secondary, "T")) != 0) goto end_free;
			current += 2;
			break;
			}
		    
		    /** else **/
		    if (check(meta_add_str(primary, "T")) != 0) goto end_free;
		    if (check(meta_add_str(secondary, "T")) != 0) goto end_free;
		    current += 1;
		    break;
		    }
		
		case 'F':
		    {
		    current += (next_char == 'F') ? 2 : 1;
		    if (check(meta_add_str(primary, "F")) != 0) goto end_free;
		    if (check(meta_add_str(secondary, "F")) != 0) goto end_free;
		    break;
		    }
		
		case 'G':
		    {
		    if (next_char == 'H')
			{
			/** 'Vghee' */
			if (current > 0 && !meta_is_vowel(original, (current - 1)))
			    {
			    if (check(meta_add_str(primary, "K")) != 0) goto end_free;
			    if (check(meta_add_str(secondary, "K")) != 0) goto end_free;
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
				    if (check(meta_add_str(primary, "J")) != 0) goto end_free;
				    if (check(meta_add_str(secondary, "J")) != 0) goto end_free;
				    }
				else
				    {
				    if (check(meta_add_str(primary, "K")) != 0) goto end_free;
				    if (check(meta_add_str(secondary, "K")) != 0) goto end_free;
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
				if (check(meta_add_str(primary, "F")) != 0) goto end_free;
				if (check(meta_add_str(secondary, "F")) != 0) goto end_free;
				}
			    else if (current > 0 && meta_get_char_at(original, (current - 1)) != 'I')
				{
				if (check(meta_add_str(primary, "K")) != 0) goto end_free;
				if (check(meta_add_str(secondary, "K")) != 0) goto end_free;
				}
			    
			    current += 2;
			    break;
			    }
			}
		    
		    if (next_char == 'N')
			{
			if (current == 1 && !is_slavo_germanic && meta_is_vowel(original, 0))
			    {
			    if (check(meta_add_str(primary, "KN")) != 0) goto end_free;
			    if (check(meta_add_str(secondary, "N")) != 0) goto end_free;
			    }
			else
			    /** not e.g. 'cagney' **/
			    if (
				next_char != 'Y'
				&& !is_slavo_germanic
				&& !meta_is_str_at(original, (current + 2), "EY", "")
			       )
				{
				if (check(meta_add_str(primary, "N")) != 0) goto end_free;
				if (check(meta_add_str(secondary, "KN")) != 0) goto end_free;
				}
			else
			    {
			    if (check(meta_add_str(primary, "KN")) != 0) goto end_free;
			    if (check(meta_add_str(secondary, "KN")) != 0) goto end_free;
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
			if (check(meta_add_str(primary, "KL")) != 0) goto end_free;
			if (check(meta_add_str(secondary, "L")) != 0) goto end_free;
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
			if (check(meta_add_str(primary, "K")) != 0) goto end_free;
			if (check(meta_add_str(secondary, "J")) != 0) goto end_free;
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
			if (check(meta_add_str(primary, "K")) != 0) goto end_free;
			if (check(meta_add_str(secondary, "J")) != 0) goto end_free;
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
			    if (check(meta_add_str(primary, "K")) != 0) goto end_free;
			    if (check(meta_add_str(secondary, "K")) != 0) goto end_free;
			    }
			else
			    {
			    /** Always soft, if french ending. **/
			    if (meta_is_str_at(original, (current + 1), "IER ", ""))
				{
				if (check(meta_add_str(primary, "J")) != 0) goto end_free;
				if (check(meta_add_str(secondary, "J")) != 0) goto end_free;
				}
			    else
				{
				if (check(meta_add_str(primary, "J")) != 0) goto end_free;
				if (check(meta_add_str(secondary, "K")) != 0) goto end_free;
				}
			    }
			current += 2;
			break;
		    }
		    
		    current += (next_char == 'G') ? 2 : 1;
		    if (check(meta_add_str(primary, "K")) != 0) goto end_free;
		    if (check(meta_add_str(secondary, "K")) != 0) goto end_free;
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
			if (check(meta_add_str(primary, "H")) != 0) goto end_free;
			if (check(meta_add_str(secondary, "H")) != 0) goto end_free;
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
			    if (check(meta_add_str(primary, "H")) != 0) goto end_free;
			    if (check(meta_add_str(secondary, "H")) != 0) goto end_free;
			    }
			else
			    {
			    if (check(meta_add_str(primary, "J")) != 0) goto end_free;
			    if (check(meta_add_str(secondary, "H")) != 0) goto end_free;
			    }
			current += 1;
			break;
			}
		    
		    if (current == 0 && !has_jose_next)
			{
			if (check(meta_add_str(primary, "J")) != 0) goto end_free; /* Yankelovich/Jankelowicz */
			if (check(meta_add_str(secondary, "A")) != 0) goto end_free;
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
			    if (check(meta_add_str(primary, "J")) != 0) goto end_free;
			    if (check(meta_add_str(secondary, "H")) != 0) goto end_free;
			    }
			else
			    {
			    if (current == last)
				{
				if (check(meta_add_str(primary, "J")) != 0) goto end_free;
				if (check(meta_add_str(secondary, "")) != 0) goto end_free;
				}
			    else
				{
				if (
				    !meta_is_str_at(original, (current + 1), "L", "T", "K", "S", "N", "M", "B", "Z", "")
				    && !meta_is_str_at(original, (current - 1), "S", "K", "L", "")
				   )
				    {
				    if (check(meta_add_str(primary, "J")) != 0) goto end_free;
				    if (check(meta_add_str(secondary, "J")) != 0) goto end_free;
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
		    if (check(meta_add_str(primary, "K")) != 0) goto end_free;
		    if (check(meta_add_str(secondary, "K")) != 0) goto end_free;
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
			    if (check(meta_add_str(primary, "L")) != 0) goto end_free;
			    if (check(meta_add_str(secondary, "")) != 0) goto end_free;
			    current += 2;
			    break;
			    }
			current += 2;
			}
		    else
			current += 1;
		    if (check(meta_add_str(primary, "L")) != 0) goto end_free;
		    if (check(meta_add_str(secondary, "L")) != 0) goto end_free;
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
		    if (check(meta_add_str(primary, "M")) != 0) goto end_free;
		    if (check(meta_add_str(secondary, "M")) != 0) goto end_free;
		    break;
		    }
		
		case 'N':
		    {
		    current += (next_char == 'N') ? 2 : 1;
		    if (check(meta_add_str(primary, "N")) != 0) goto end_free;
		    if (check(meta_add_str(secondary, "N")) != 0) goto end_free;
		    break;
		    }
		
		case 'P':
		    {
		    if (next_char == 'H')
			{
			if (check(meta_add_str(primary, "F")) != 0) goto end_free;
			if (check(meta_add_str(secondary, "F")) != 0) goto end_free;
			current += 2;
			break;
			}
		    
		    /** Also account for "campbell", "raspberry" **/
		    current += (meta_is_str_at(original, (current + 1), "P", "B", "")) ? 2 : 1;
		    if (check(meta_add_str(primary, "P")) != 0) goto end_free;
		    if (check(meta_add_str(secondary, "P")) != 0) goto end_free;
		    break;
		    }
		
		case 'Q':
		    {
		    current += (next_char == 'Q') ? 2 : 1;
		    if (check(meta_add_str(primary, "K")) != 0) goto end_free;
		    if (check(meta_add_str(secondary, "K")) != 0) goto end_free;
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
		    
		    if (check(meta_add_str(primary, (no_primary) ? "" : "R")) != 0) goto end_free;
		    if (check(meta_add_str(secondary, "R")) != 0) goto end_free;
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
			if (check(meta_add_str(primary, "X")) != 0) goto end_free;
			if (check(meta_add_str(secondary, "S")) != 0) goto end_free;
			current += 1;
			break;
			}
		    
		    if (meta_is_str_at(original, current, "SH", ""))
			{
			const bool germanic = meta_is_str_at(original, (current + 1), "HEIM", "HOEK", "HOLM", "HOLZ", "");
			const char* sound = (germanic) ? "S" : "X";
			if (check(meta_add_str(primary, sound)) != 0) goto end_free;
			if (check(meta_add_str(secondary, sound)) != 0) goto end_free;
			current += 2;
			break;
			}
		    
		    /** Italian & Armenian. **/
		    if (meta_is_str_at(original, current, "SIO", "SIA", "SIAN", ""))
			{
			if (check(meta_add_str(primary, "S")) != 0) goto end_free;
			if (check(meta_add_str(secondary, (is_slavo_germanic) ? "S" : "X")) != 0) goto end_free;
			current += 3;
			break;
			}
		    
		    /** german & anglicisations, e.g. 'smith' match 'schmidt', 'snider' match 'schneider' **/
		    /** also, -sz- in slavic language although in hungarian it is pronounced 's' **/
		    if (current == 0 && meta_is_str_at(original, (current + 1), "M", "N", "L", "W", ""))
			{
			if (check(meta_add_str(primary, "S")) != 0) goto end_free;
			if (check(meta_add_str(secondary, "X")) != 0) goto end_free;
			current += 1;
			break;
			}
		    if (meta_is_str_at(original, (current + 1), "Z", ""))
			{
			if (check(meta_add_str(primary, "S")) != 0) goto end_free;
			if (check(meta_add_str(secondary, "X")) != 0) goto end_free;
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
				if (check(meta_add_str(primary, (x_sound) ? "X" : "SK")) != 0) goto end_free;
				if (check(meta_add_str(secondary, "SK")) != 0) goto end_free;
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
				if (check(meta_add_str(primary, "X")) != 0) goto end_free;
				if (check(meta_add_str(secondary, (s_sound) ? "S" : "X")) != 0) goto end_free;
				current += 3;
				break;
				}
			    }
			
			/** Default case. **/
			const char* sound = (meta_is_str_at(original, (current + 2), "E", "I", "Y", "")) ? "S" : "SK";
			if (check(meta_add_str(primary, sound)) != 0) goto end_free;
			if (check(meta_add_str(secondary, sound)) != 0) goto end_free;
			current += 3;
			break;
			}
		    
		    /** French e.g. 'resnais', 'artois' **/
		    const bool no_primary = (current == last && meta_is_str_at(original, (current - 2), "AI", "OI", ""));
		    if (check(meta_add_str(primary, (no_primary) ? "" : "S")) != 0) goto end_free;
		    if (check(meta_add_str(secondary, "S")) != 0) goto end_free;
		    current += (meta_is_str_at(original, (current + 1), "S", "Z", "")) ? 2 : 1;
		    break;
		    }
		
		case 'T':
		    {
		    if (meta_is_str_at(original, current, "TIA", "TCH", "TION", ""))
			{
			if (check(meta_add_str(primary, "X")) != 0) goto end_free;
			if (check(meta_add_str(secondary, "X")) != 0) goto end_free;
			current += 3;
			break;
			}
		    
		    if (meta_is_str_at(original, current, "TH", "TTH", ""))
			{
			/** Special case 'thomas', 'thames' or germanic. **/
			char* primary_char = (
			    meta_is_str_at(original, (current + 2), "OM", "AM", "")
			    || meta_is_str_at(original, 0, "SCH", "VAN ", "VON ", "")
			) ? "T" : "0"; /* Zero, not O. */
			
			if (check(meta_add_str(primary, primary_char)) != 0) goto end_free; 
			if (check(meta_add_str(secondary, "T")) != 0) goto end_free;
			current += 2;
			break;
			}
		    
		    if (check(meta_add_str(primary, "T")) != 0) goto end_free;
		    if (check(meta_add_str(secondary, "T")) != 0) goto end_free;
		    current += (meta_is_str_at(original, (current + 1), "T", "D", "")) ? 2 : 1;
		    break;
		    }
		
		case 'V':
		    {
		    if (check(meta_add_str(primary, "F")) != 0) goto end_free;
		    if (check(meta_add_str(secondary, "F")) != 0) goto end_free;
		    current += (next_char == 'V') ? 2 : 1;
		    break;
		    }
		
		case 'W':
		    {
		    /** Can also be in middle of word. **/
		    if (meta_is_str_at(original, current, "WR", ""))
			{
			if (check(meta_add_str(primary, "R")) != 0) goto end_free;
			if (check(meta_add_str(secondary, "R")) != 0) goto end_free;
			current += 2;
			break;
			}
		    
		    const bool next_is_vowel = meta_is_vowel(original, current + 1);
		    if (current == 0 && (next_is_vowel || meta_is_str_at(original, current, "WH", "")))
			{
			/** Wasserman should match Vasserman. **/
			if (check(meta_add_str(primary, "A")) != 0) goto end_free;
			if (check(meta_add_str(secondary, (next_is_vowel) ? "F" : "A")) != 0) goto end_free;
			}
		    
		    /** Arnow should match Arnoff. **/
		    if ((current == last && meta_is_vowel(original, current - 1))
			|| meta_is_str_at(original, (current - 1), "EWSKI", "EWSKY", "OWSKI", "OWSKY", "")
			|| meta_is_str_at(original, 0, "SCH", "")
		       )
			{
			if (check(meta_add_str(primary, "")) != 0) goto end_free;
			if (check(meta_add_str(secondary, "F")) != 0) goto end_free;
			current += 1;
			break;
			}
		    
		    /** Polish e.g. 'filipowicz' **/
		    if (meta_is_str_at(original, current, "WICZ", "WITZ", ""))
			{
			if (check(meta_add_str(primary, "TS")) != 0) goto end_free;
			if (check(meta_add_str(secondary, "FX")) != 0) goto end_free;
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
			if (check(meta_add_str(primary, "KS")) != 0) goto end_free;
			if (check(meta_add_str(secondary, "KS")) != 0) goto end_free;
			}
		    
		    current += (meta_is_str_at(original, (current + 1), "C", "X", "")) ? 2 : 1;
		    break;
		    }
		
		case 'Z':
		    {
		    /** Chinese pinyin e.g. 'zhao' **/
		    if (next_char == 'H')
			{
			if (check(meta_add_str(primary, "J")) != 0) goto end_free;
			if (check(meta_add_str(secondary, "J")) != 0) goto end_free;
			current += 2;
			break;
			}
		    
		    const bool has_t_sound = (
			meta_is_str_at(original, (current + 1), "ZO", "ZI", "ZA", "")
			|| (is_slavo_germanic && current > 0 && meta_get_char_at(original, (current - 1)) != 'T')
		    );
		    if (check(meta_add_str(primary, "S")) != 0) goto end_free;
		    if (check(meta_add_str(secondary, (has_t_sound) ? "TS" : "S")) != 0) goto end_free;
		    current += (next_char == 'Z') ? 2 : 1;
		    break;
		    }
		
		default:
		    current += 1;
		}
	    }
	
	/** Write the output strings. **/
	*primary_code = primary->str;
	*secondary_code = secondary->str;
	primary->free_str_on_destroy = 0;
	secondary->free_str_on_destroy = 0;
	ret = 0;
	
    end_free:
	if (UNLIKELY(ret != 0)) fprintf(stderr, "Error: meta_double_metaphone() failed (error code %d).\n", ret);
	meta_destroy_string(original);
	meta_destroy_string(primary);
	meta_destroy_string(secondary);
	
	return ret;
    }
