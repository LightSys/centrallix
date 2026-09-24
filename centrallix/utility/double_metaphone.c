/************************************************************************/
/* Text-DoubleMetaphone							*/
/* Centrallix Core							*/
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
/* this information, please use the link provided above.		*/
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
/*    1: https://github.com/gitpan/Text-DoubleMetaphone			*/
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
/* 		written by Philips. This implementation was written by	*/
/* 		Maurice Aubrey for C/C++ with bug fixes provided by	*/
/* 		Kevin Atkinson. It was heavily revised by Israel Fuller	*/
/* 		to align with the Centrallix coding style and make use	*/
/* 		of the centrallix libraries.				*/
/************************************************************************/

/*** Note to future programmers reading this file (by Israel Fuller):
 *** 
 *** This file was copied from a GitHub Repo with licensing (see above).
 *** 
 *** As for this code, I've modified it to use styling, memory management, and
 *** libraries used to be consistent with Centrallix.  I also wrote comments and
 *** tests based on my own understanding, so they might not accurately reflect
 *** the original author's intent.
 *** 
 *** To be honest, trying to make this code as readable as possible was by no
 *** means easy, due to the complex boolean algebra.  If a linguist ever reads
 *** this, please factor out some logic into local variables with descriptive
 *** names so that the rest of us can read this without our eyes glazing over.
 *** 
 *** If you have any questions, please feel free to reach out to me or Greg.
 *** 
 *** Original Source: https://github.com/gitpan/Text-DoubleMetaphone
 ***/

#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "cxlib/newmalloc.h"
#include "cxlib/xstring.h"
#include "cxlib/warn.h"
#include "cxlib/expect.h"
#include "double_metaphone.h"

/*** Convert all characters of an XString to uppercase.
 *** 
 *** @param s The XString being modified.
 *** @returns 0 if successful, or -1 if an error occurs.
 ***/
static int
meta_i_makeUpper(pXString s)
    {
	/** Handle edge cases. **/
	if (UNLIKELY(s == NULL)) return -1;
	
	/** Get the xstring character buffer. **/
	char* buf = xsString(s);
	const int length = xsLength(s);
	if (UNLIKELY(buf == NULL)) return -1;
	if (UNLIKELY(length < 0)) return -1;
	
	/** Uppercase each character. **/
	for (int i = 0; i < length; i++)
	    buf[i] = (char)toupper((unsigned char)buf[i]);
    
    return 0;
    }

/*** @param s The XString being checked.
 *** @param pos The character location to check within the XString.
 *** @returns The character at the position in the XString, or
 ***          '\0' if the position is not in the XString.
 ***/
static char
meta_i_getCharAt(pXString s, unsigned int pos)
    {
	/** Handle edge cases. **/
	if (UNLIKELY(s == NULL)) return '\0';
	
	/** A position past INT_MAX has wrapped, so it is out of bounds. **/
	if (UNLIKELY(pos > (unsigned int)INT_MAX)) return '\0';
    
    return xsCharAt(s, (int)pos);
    }

/*** Checks if a character in an XString is a vowel.
 *** 
 *** @param s The XString being checked.
 *** @param pos The character location to check within the XString.
 ***/
static bool
meta_i_isVowel(pXString s, unsigned int pos)
    {
	const char c = meta_i_getCharAt(s, pos);
    
    return ((c == 'A') || (c == 'E') || (c == 'I') ||
	    (c == 'O') || (c == 'U') || (c == 'Y'));
    }

/*** Search an XString for "W", "K", or "CZ", which indicate that the
 *** string is Slavo Germanic.
 *** 
 *** @param s The XString to be searched.
 *** @returns 1 if the XString is Slavo Germanic, or 0 otherwise.
 ***/
static bool
meta_i_isSlavoGermanic(pXString s)
    {
	/** Handle edge cases. **/
	if (UNLIKELY(s == NULL)) return false;
    
    return (xsFind(s, "W", 1, 0) >= 0)
	|| (xsFind(s, "K", 1, 0) >= 0)
	|| (xsFind(s, "CZ", 2, 0) >= 0);
    }

/*** Checks to see if any of a list of strings appear in the given
 *** XString at the given start position.
 *** 
 *** @attention - Note that the START value is 0 based.
 *** 
 *** @param s The XString being checked.
 *** @param start The zero-based position at which to begin searching
 *** 	within the XString.
 *** @returns 1 if any of the character sequences appear at the start
 *** 	in the XString and 0 otherwise.
 ***/
static bool
meta_i_isStrAt(pXString s, unsigned int start, ...)
    {
    va_list ap;
    bool found = false;
    
	va_start(ap, start);
	
	char* test;
	do
	    {
	    /** An empty string terminates the argument list. **/
	    test = va_arg(ap, char*);
	    if (test[0] == '\0')
		break;
	    
	    /** Reading through meta_i_getCharAt() cannot run off the end. **/
	    int i;
	    for (i = 0; test[i] != '\0'; i++)
		{
		if (meta_i_getCharAt(s, start + (unsigned int)i) != test[i])
		    break;
		}
	    if (test[i] == '\0')
		{
		found = true;
		break;
		}
	    }
	while (true);
	
	va_end(ap);
    
    return found;
    }

/*** Computes double metaphone.
 *** 
 *** On success the caller owns both strings and must release them with
 *** nmSysFree().  On failure neither pointer is written.
 *** 
 *** Example Usage:
 *** ```c
 *** char* primary_code;
 *** char* secondary_code;
 *** if (metaDoubleMetaphone(input, &primary_code, &secondary_code) != 0) return -1;
 *** 
 *** printf("%s %s\n", primary_code, secondary_code);
 *** 
 *** nmSysFree(primary_code);
 *** nmSysFree(secondary_code);
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
metaDoubleMetaphone(const char* str, char** primary_code, char** secondary_code)
    {
    int ret = -1;
    XString original, primary, secondary;
    
	/** xsInit() always sets .String, so NULL marks an uninitialized xstring. **/
	original.String = NULL;
	primary.String = NULL;
	secondary.String = NULL;
	
	if (UNLIKELY(xsInit(&original) != 0)) goto end_free;
	if (UNLIKELY(xsInit(&primary) != 0)) goto end_free;
	if (UNLIKELY(xsInit(&secondary) != 0)) goto end_free;
	
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
	
	/** Uppercase the input, and pad it so we can index beyond the end. **/
	if (xsConcatenate(&original, (char*)str, (int)length) < 0) goto end_free;
	if (meta_i_makeUpper(&original) != 0) goto end_free;
	if (xsConcatenateLiteral(&original, "     ") < 0) goto end_free;
	
	/** Skip these if they are at start of a word. **/
	if (meta_i_isStrAt(&original, 0, "GN", "KN", "PN", "WR", "PS", ""))
	    current += 1;
	
	/** Initial 'X' is pronounced 'Z' e.g. 'Xavier' **/
	const char first_char = meta_i_getCharAt(&original, 0);
	if (first_char == 'X')
	    {
	    if (xsConcatenateLiteral(&primary, "S") < 0) goto end_free; /* 'Z' maps to 'S' */
	    if (xsConcatenateLiteral(&secondary, "S") < 0) goto end_free;
	    current += 1;
	    }
	
	/** Precomputing this is useful. **/
	const bool is_slavo_germanic = meta_i_isSlavoGermanic(&original);
	
	/** Main loop. **/
	while (current < length)
	    {
	    const char cur_char = meta_i_getCharAt(&original, current);
	    const char next_char = meta_i_getCharAt(&original, current + 1);
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
			if (xsConcatenateLiteral(&primary, "A") < 0) goto end_free;
			if (xsConcatenateLiteral(&secondary, "A") < 0) goto end_free;
			}
		    current += 1;
		    break;
		    }
		
		case 'B':
		    {
		    /** "-mb", e.g. "dumb", already skipped over... **/
		    if (xsConcatenateLiteral(&primary, "P") < 0) goto end_free;
		    if (xsConcatenateLiteral(&secondary, "P") < 0) goto end_free;
		    
		    current += (next_char == 'B') ? 2 : 1;
		    break;
		    }
		
		case 'C':
		    {
		    /** Various germanic. **/
		    if (
			(current > 1)
			&& !meta_i_isVowel(&original, current - 2)
			&& meta_i_isStrAt(&original, (current - 1), "ACH", "")
			&& meta_i_getCharAt(&original, current + 2) != 'I'
			&& (
			    meta_i_getCharAt(&original, current + 2) != 'E'
			    || meta_i_isStrAt(&original, (current - 2), "BACHER", "MACHER", "")
			)
		       )
			{
			if (xsConcatenateLiteral(&primary, "K") < 0) goto end_free;
			if (xsConcatenateLiteral(&secondary, "K") < 0) goto end_free;
			current += 2;
			break;
			}
		    
		    /** Special case 'caesar' **/
		    if (current == 0 && meta_i_isStrAt(&original, current, "CAESAR", ""))
			{
			if (xsConcatenateLiteral(&primary, "S") < 0) goto end_free;
			if (xsConcatenateLiteral(&secondary, "S") < 0) goto end_free;
			current += 2;
			break;
			}
		    
		    /** Italian 'chianti' **/
		    if (meta_i_isStrAt(&original, current, "CHIA", ""))
			{
			if (xsConcatenateLiteral(&primary, "K") < 0) goto end_free;
			if (xsConcatenateLiteral(&secondary, "K") < 0) goto end_free;
			current += 2;
			break;
			}
		    
		    if (meta_i_isStrAt(&original, current, "CH", ""))
			{
			/** Find 'michael' **/
			if (current > 0 && meta_i_isStrAt(&original, current, "CHAE", ""))
			    {
			    if (xsConcatenateLiteral(&primary, "K") < 0) goto end_free;
			    if (xsConcatenateLiteral(&secondary, "X") < 0) goto end_free;
			    current += 2;
			    break;
			    }
			
			/** Greek roots e.g. 'chemistry', 'chorus' **/
			if (
			    current == 0
			    && meta_i_isStrAt(&original, (current + 1), "HOR", "HYM", "HIA", "HEM", "HARAC", "HARIS", "")
			    && !meta_i_isStrAt(&original, 0, "CHORE", "")
			   )
			    {
			    if (xsConcatenateLiteral(&primary, "K") < 0) goto end_free;
			    if (xsConcatenateLiteral(&secondary, "K") < 0) goto end_free;
			    current += 2;
			    break;
			    }
			
			/** Germanic, greek, or otherwise 'ch' for 'kh' sound. **/
			if (
			    meta_i_isStrAt(&original, 0, "SCH", "VAN ", "VON ", "")
			    /** 'architect' but not 'arch', 'orchestra', 'orchid' **/
			    || meta_i_isStrAt(&original, (current - 2), "ORCHES", "ARCHIT", "ORCHID", "")
			    || meta_i_isStrAt(&original, (current + 2), "T", "S", "")
			    || (
				(current == 0 || meta_i_isStrAt(&original, (current - 1), "A", "O", "U", "E", ""))
				/** e.g., 'wachtler', 'wechsler', but not 'tichner' **/
				&& meta_i_isStrAt(&original, (current + 2), "L", "R", "N", "M", "B", "H", "F", "V", "W", " ", "")
			       )
			   )
			    {
			    if (xsConcatenateLiteral(&primary, "K") < 0) goto end_free;
			    if (xsConcatenateLiteral(&secondary, "K") < 0) goto end_free;
			    }
			else
			    {
			    if (current > 0)
				{
				if (meta_i_isStrAt(&original, 0, "MC", ""))
				    {
				    /* e.g., "McHugh" */
				    if (xsConcatenateLiteral(&primary, "K") < 0) goto end_free;
				    if (xsConcatenateLiteral(&secondary, "K") < 0) goto end_free;
				    }
				else
				    {
				    if (xsConcatenateLiteral(&primary, "X") < 0) goto end_free;
				    if (xsConcatenateLiteral(&secondary, "K") < 0) goto end_free;
				    }
				}
			    else
				{
				if (xsConcatenateLiteral(&primary, "X") < 0) goto end_free;
				if (xsConcatenateLiteral(&secondary, "X") < 0) goto end_free;
				}
			    }
			current += 2;
			break;
			}
		    
		    /** e.g., 'czerny' **/
		    if (meta_i_isStrAt(&original, current, "CZ", "")
			&& !meta_i_isStrAt(&original, (current - 2), "WICZ", ""))
			{
			if (xsConcatenateLiteral(&primary, "S") < 0) goto end_free;
			if (xsConcatenateLiteral(&secondary, "X") < 0) goto end_free;
			current += 2;
			break;
			}
		    
		    /** e.g., 'focaccia' **/
		    if (meta_i_isStrAt(&original, (current + 1), "CIA", ""))
			{
			if (xsConcatenateLiteral(&primary, "X") < 0) goto end_free;
			if (xsConcatenateLiteral(&secondary, "X") < 0) goto end_free;
			current += 3;
			break;
			}
		    
		    /** Double 'C' rule. **/
		    if (
			meta_i_isStrAt(&original, current, "CC", "")
			&& !(current == 1 && first_char == 'M') /* McClellan exception. */
		       )
			{
			/** 'bellocchio' but not 'bacchus' **/
			if (
			    meta_i_isStrAt(&original, (current + 2), "I", "E", "H", "")
			    && !meta_i_isStrAt(&original, (current + 2), "HU", "")
			   )
			    {
			    /** 'accident', 'accede' 'succeed' **/
			    if (
				(current == 1 && meta_i_getCharAt(&original, current - 1) == 'A')
				|| meta_i_isStrAt(&original, (current - 1), "UCCEE", "UCCES", "")
			       )
				{
				if (xsConcatenateLiteral(&primary, "KS") < 0) goto end_free;
				if (xsConcatenateLiteral(&secondary, "KS") < 0) goto end_free;
				/** 'bacci', 'bertucci', other italian **/
				}
			    else
				{
				if (xsConcatenateLiteral(&primary, "X") < 0) goto end_free;
				if (xsConcatenateLiteral(&secondary, "X") < 0) goto end_free;
				}
			    current += 3;
			    break;
			    }
			else
			    { /** Pierce's rule **/
			    if (xsConcatenateLiteral(&primary, "K") < 0) goto end_free;
			    if (xsConcatenateLiteral(&secondary, "K") < 0) goto end_free;
			    current += 2;
			    break;
			    }
			}
		    
		    if (meta_i_isStrAt(&original, current, "CK", "CG", "CQ", ""))
			{
			if (xsConcatenateLiteral(&primary, "K") < 0) goto end_free;
			if (xsConcatenateLiteral(&secondary, "K") < 0) goto end_free;
			current += 2;
			break;
			}
		    
		    if (meta_i_isStrAt(&original, current, "CI", "CE", "CY", ""))
			{
			/* Italian vs. English */
			if (meta_i_isStrAt(&original, current, "CIO", "CIE", "CIA", ""))
			    {
			    if (xsConcatenateLiteral(&primary, "S") < 0) goto end_free;
			    if (xsConcatenateLiteral(&secondary, "X") < 0) goto end_free;
			    }
			else
			    {
			    if (xsConcatenateLiteral(&primary, "S") < 0) goto end_free;
			    if (xsConcatenateLiteral(&secondary, "S") < 0) goto end_free;
			    }
			current += 2;
			break;
			}
		    
		    /** else **/
		    if (xsConcatenateLiteral(&primary, "K") < 0) goto end_free;
		    if (xsConcatenateLiteral(&secondary, "K") < 0) goto end_free;
		    
		    /** Name sent in 'mac caffrey', 'mac gregor' **/
		    if (meta_i_isStrAt(&original, (current + 1), " C", " Q", " G", ""))
			current += 3;
		    else if (meta_i_isStrAt(&original, (current + 1), "C", "K", "Q", "")
			     && !meta_i_isStrAt(&original, (current + 1), "CE", "CI", ""))
			current += 2;
		    else
			current += 1;
		    break;
		    }
		
		case 'D':
		    {
		    if (meta_i_isStrAt(&original, current, "DG", ""))
			{
			if (meta_i_isStrAt(&original, (current + 2), "I", "E", "Y", ""))
			    {
			    /** e.g. 'edge' **/
			    if (xsConcatenateLiteral(&primary, "J") < 0) goto end_free;
			    if (xsConcatenateLiteral(&secondary, "J") < 0) goto end_free;
			    current += 3;
			    break;
			    }
			else
			    {
			    /** e.g. 'edgar' **/
			    if (xsConcatenateLiteral(&primary, "TK") < 0) goto end_free;
			    if (xsConcatenateLiteral(&secondary, "TK") < 0) goto end_free;
			    current += 2;
			    break;
			    }
			}
		    
		    if (meta_i_isStrAt(&original, current, "DT", "DD", ""))
			{
			if (xsConcatenateLiteral(&primary, "T") < 0) goto end_free;
			if (xsConcatenateLiteral(&secondary, "T") < 0) goto end_free;
			current += 2;
			break;
			}
		    
		    /** else **/
		    if (xsConcatenateLiteral(&primary, "T") < 0) goto end_free;
		    if (xsConcatenateLiteral(&secondary, "T") < 0) goto end_free;
		    current += 1;
		    break;
		    }
		
		case 'F':
		    {
		    current += (next_char == 'F') ? 2 : 1;
		    if (xsConcatenateLiteral(&primary, "F") < 0) goto end_free;
		    if (xsConcatenateLiteral(&secondary, "F") < 0) goto end_free;
		    break;
		    }
		
		case 'G':
		    {
		    if (next_char == 'H')
			{
			/** 'Vghee' **/
			if (current > 0 && !meta_i_isVowel(&original, (current - 1)))
			    {
			    if (xsConcatenateLiteral(&primary, "K") < 0) goto end_free;
			    if (xsConcatenateLiteral(&secondary, "K") < 0) goto end_free;
			    current += 2;
			    break;
			    }
			
			if (current < 3)
			    {
			    /** 'ghislane', 'ghiradelli' **/
			    if (current == 0)
				{
				if (meta_i_getCharAt(&original, (current + 2)) == 'I')
				    {
				    if (xsConcatenateLiteral(&primary, "J") < 0) goto end_free;
				    if (xsConcatenateLiteral(&secondary, "J") < 0) goto end_free;
				    }
				else
				    {
				    if (xsConcatenateLiteral(&primary, "K") < 0) goto end_free;
				    if (xsConcatenateLiteral(&secondary, "K") < 0) goto end_free;
				    }
				current += 2;
				break;
				}
			    }
			
			if (
			    /** Parker's rule (with some further refinements) - e.g., 'hugh' **/
			    (current > 1 && meta_i_isStrAt(&original, (current - 2), "B", "H", "D", ""))
			    /** e.g., 'bough' **/
			    || (current > 2 && meta_i_isStrAt(&original, (current - 3), "B", "H", "D", ""))
			    /** e.g., 'broughton' **/
			    || (current > 3 && meta_i_isStrAt(&original, (current - 4), "B", "H", ""))
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
				&& meta_i_getCharAt(&original, (current - 1)) == 'U'
				&& meta_i_isStrAt(&original, (current - 3), "C", "G", "L", "R", "T", "")
			       )
				{
				if (xsConcatenateLiteral(&primary, "F") < 0) goto end_free;
				if (xsConcatenateLiteral(&secondary, "F") < 0) goto end_free;
				}
			    else if (current > 0 && meta_i_getCharAt(&original, (current - 1)) != 'I')
				{
				if (xsConcatenateLiteral(&primary, "K") < 0) goto end_free;
				if (xsConcatenateLiteral(&secondary, "K") < 0) goto end_free;
				}
			    
			    current += 2;
			    break;
			    }
			}
		    
		    if (next_char == 'N')
			{
			if (current == 1 && !is_slavo_germanic && meta_i_isVowel(&original, 0))
			    {
			    if (xsConcatenateLiteral(&primary, "KN") < 0) goto end_free;
			    if (xsConcatenateLiteral(&secondary, "N") < 0) goto end_free;
			    }
			else
			    /** not e.g. 'cagney' **/
			    if (
				next_char != 'Y'
				&& !is_slavo_germanic
				&& !meta_i_isStrAt(&original, (current + 2), "EY", "")
			       )
				{
				if (xsConcatenateLiteral(&primary, "N") < 0) goto end_free;
				if (xsConcatenateLiteral(&secondary, "KN") < 0) goto end_free;
				}
			else
			    {
			    if (xsConcatenateLiteral(&primary, "KN") < 0) goto end_free;
			    if (xsConcatenateLiteral(&secondary, "KN") < 0) goto end_free;
			    }
			current += 2;
			break;
			}
		    
		    /** 'tagliaro' **/
		    if (
			!is_slavo_germanic
			&& meta_i_isStrAt(&original, (current + 1), "LI", "")
		       )
			{
			if (xsConcatenateLiteral(&primary, "KL") < 0) goto end_free;
			if (xsConcatenateLiteral(&secondary, "L") < 0) goto end_free;
			current += 2;
			break;
			}
		    
		    /** -ges-,-gep-,-gel-, -gie- at beginning **/
		    if (
			current == 0
			&& (
			    next_char == 'Y'
			    || meta_i_isStrAt(
				&original, (current + 1),
				"ES", "EP", "EB", "EL", "EY", "IB",
				"IL", "IN", "IE", "EI", "ER", ""
			    )
			   )
		       )
			{
			if (xsConcatenateLiteral(&primary, "K") < 0) goto end_free;
			if (xsConcatenateLiteral(&secondary, "J") < 0) goto end_free;
			current += 2;
			break;
			}
		    
		    /** -ger-, -gy- **/
		    if (
			(next_char == 'Y' || meta_i_isStrAt(&original, (current + 1), "ER", ""))
			/** Exceptions. **/
			&& !meta_i_isStrAt(&original, 0, "DANGER", "RANGER", "MANGER", "")
			&& !meta_i_isStrAt(&original, (current - 1), "E", "I", "RGY", "OGY", "")
		       )
			{
			if (xsConcatenateLiteral(&primary, "K") < 0) goto end_free;
			if (xsConcatenateLiteral(&secondary, "J") < 0) goto end_free;
			current += 2;
			break;
			}
		    
		    /** Italian e.g., 'biaggi' **/
		    if (
			meta_i_isStrAt(&original, (current + 1), "E", "I", "Y", "")
			|| meta_i_isStrAt(&original, (current - 1), "AGGI", "OGGI", "")
		       )
			{
			/** Obvious germanic. **/
			if (meta_i_isStrAt(&original, 0, "SCH", "VAN ", "VON ", "")
			    || meta_i_isStrAt(&original, (current + 1), "ET", ""))
			    {
			    if (xsConcatenateLiteral(&primary, "K") < 0) goto end_free;
			    if (xsConcatenateLiteral(&secondary, "K") < 0) goto end_free;
			    }
			else
			    {
			    /** Always soft, if french ending. **/
			    if (meta_i_isStrAt(&original, (current + 1), "IER ", ""))
				{
				if (xsConcatenateLiteral(&primary, "J") < 0) goto end_free;
				if (xsConcatenateLiteral(&secondary, "J") < 0) goto end_free;
				}
			    else
				{
				if (xsConcatenateLiteral(&primary, "J") < 0) goto end_free;
				if (xsConcatenateLiteral(&secondary, "K") < 0) goto end_free;
				}
			    }
			current += 2;
			break;
			}
		    
		    current += (next_char == 'G') ? 2 : 1;
		    if (xsConcatenateLiteral(&primary, "K") < 0) goto end_free;
		    if (xsConcatenateLiteral(&secondary, "K") < 0) goto end_free;
		    break;
		    }
		
		case 'H':
		    {
		    /** Only keep if first & before vowel or between 2 vowels. **/
		    if (
			(current == 0 || meta_i_isVowel(&original, (current - 1)))
			&& meta_i_isVowel(&original, current + 1)
		       )
			{
			if (xsConcatenateLiteral(&primary, "H") < 0) goto end_free;
			if (xsConcatenateLiteral(&secondary, "H") < 0) goto end_free;
			current += 2;
			}
		    else /* also takes care of 'HH' */
			current += 1;
		    break;
		    }
		
		case 'J':
		    {
		    /** Obvious spanish, 'jose', 'san jacinto' **/
		    const bool has_jose_next = meta_i_isStrAt(&original, current, "JOSE", "");
		    const bool starts_with_san = meta_i_isStrAt(&original, 0, "SAN ", "");
		    if (has_jose_next || starts_with_san)
			{
			if (
			    starts_with_san
			    /** I don't know what this condition means. **/
			    || (current == 0 && meta_i_getCharAt(&original, current + 4) == ' ')
			   )
			    {
			    if (xsConcatenateLiteral(&primary, "H") < 0) goto end_free;
			    if (xsConcatenateLiteral(&secondary, "H") < 0) goto end_free;
			    }
			else
			    {
			    if (xsConcatenateLiteral(&primary, "J") < 0) goto end_free;
			    if (xsConcatenateLiteral(&secondary, "H") < 0) goto end_free;
			    }
			current += 1;
			break;
			}
		    
		    if (current == 0 && !has_jose_next)
			{
			if (xsConcatenateLiteral(&primary, "J") < 0) goto end_free; /* Yankelovich/Jankelowicz */
			if (xsConcatenateLiteral(&secondary, "A") < 0) goto end_free;
			}
		    else
			{
			/** spanish pron. of e.g. 'bajador' **/
			if (
			    !is_slavo_germanic
			    && (next_char == 'A' || next_char == 'O')
			    && meta_i_isVowel(&original, (current - 1))
			   )
			    {
			    if (xsConcatenateLiteral(&primary, "J") < 0) goto end_free;
			    if (xsConcatenateLiteral(&secondary, "H") < 0) goto end_free;
			    }
			else
			    {
			    if (current == last)
				{
				if (xsConcatenateLiteral(&primary, "J") < 0) goto end_free;
				if (xsConcatenateLiteral(&secondary, "") < 0) goto end_free;
				}
			    else
				{
				if (
				    !meta_i_isStrAt(&original, (current + 1), "L", "T", "K", "S", "N", "M", "B", "Z", "")
				    && !meta_i_isStrAt(&original, (current - 1), "S", "K", "L", "")
				   )
				    {
				    if (xsConcatenateLiteral(&primary, "J") < 0) goto end_free;
				    if (xsConcatenateLiteral(&secondary, "J") < 0) goto end_free;
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
		    if (xsConcatenateLiteral(&primary, "K") < 0) goto end_free;
		    if (xsConcatenateLiteral(&secondary, "K") < 0) goto end_free;
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
			     && meta_i_isStrAt(&original, (current - 1), "ILLO", "ILLA", "ALLE", "")
			    )
			    || (
				meta_i_isStrAt(&original, (current - 1), "ALLE", "")
				&& (
				    meta_i_isStrAt(&original, (last - 1), "AS", "OS", "")
				    || meta_i_isStrAt(&original, last, "A", "O", "")
				   )
			       )
			   )
			    {
			    if (xsConcatenateLiteral(&primary, "L") < 0) goto end_free;
			    if (xsConcatenateLiteral(&secondary, "") < 0) goto end_free;
			    current += 2;
			    break;
			    }
			current += 2;
			}
		    else
			current += 1;
		    if (xsConcatenateLiteral(&primary, "L") < 0) goto end_free;
		    if (xsConcatenateLiteral(&secondary, "L") < 0) goto end_free;
		    break;
		    }
		
		case 'M':
		    {
		    current += (
			(
			 meta_i_isStrAt(&original, (current - 1), "UMB", "")
			 && (current + 1 == last || meta_i_isStrAt(&original, (current + 2), "ER", ""))
			)
			/** 'dumb','thumb' **/
			|| next_char == 'M'
		    ) ? 2 : 1;
		    if (xsConcatenateLiteral(&primary, "M") < 0) goto end_free;
		    if (xsConcatenateLiteral(&secondary, "M") < 0) goto end_free;
		    break;
		    }
		
		case 'N':
		    {
		    current += (next_char == 'N') ? 2 : 1;
		    if (xsConcatenateLiteral(&primary, "N") < 0) goto end_free;
		    if (xsConcatenateLiteral(&secondary, "N") < 0) goto end_free;
		    break;
		    }
		
		case 'P':
		    {
		    if (next_char == 'H')
			{
			if (xsConcatenateLiteral(&primary, "F") < 0) goto end_free;
			if (xsConcatenateLiteral(&secondary, "F") < 0) goto end_free;
			current += 2;
			break;
			}
		    
		    /** Also account for "campbell", "raspberry" **/
		    current += (meta_i_isStrAt(&original, (current + 1), "P", "B", "")) ? 2 : 1;
		    if (xsConcatenateLiteral(&primary, "P") < 0) goto end_free;
		    if (xsConcatenateLiteral(&secondary, "P") < 0) goto end_free;
		    break;
		    }
		
		case 'Q':
		    {
		    current += (next_char == 'Q') ? 2 : 1;
		    if (xsConcatenateLiteral(&primary, "K") < 0) goto end_free;
		    if (xsConcatenateLiteral(&secondary, "K") < 0) goto end_free;
		    break;
		    }
		
		case 'R':
		    {
		    /** French e.g. 'rogier', but exclude 'hochmeier' **/
		    const bool no_primary = (
			!is_slavo_germanic
			&& current == last
			&& meta_i_isStrAt(&original, (current - 2), "IE", "")
			&& !meta_i_isStrAt(&original, (current - 4), "ME", "MA", "")
		    );
		    
		    if (xsConcatenate(&primary, (no_primary) ? "" : "R", -1) < 0) goto end_free;
		    if (xsConcatenateLiteral(&secondary, "R") < 0) goto end_free;
		    current += (next_char == 'R') ? 2 : 1;
		    break;
		    }
		
		case 'S':
		    {
		    /** Special cases 'island', 'isle', 'carlisle', 'carlysle' **/
		    if (meta_i_isStrAt(&original, (current - 1), "ISL", "YSL", ""))
			{
			current += 1;
			break;
			}
		    
		    /** Special case 'sugar-' **/
		    if (current == 0 && meta_i_isStrAt(&original, current, "SUGAR", ""))
			{
			if (xsConcatenateLiteral(&primary, "X") < 0) goto end_free;
			if (xsConcatenateLiteral(&secondary, "S") < 0) goto end_free;
			current += 1;
			break;
			}
		    
		    if (meta_i_isStrAt(&original, current, "SH", ""))
			{
			const bool germanic = meta_i_isStrAt(&original, (current + 1), "HEIM", "HOEK", "HOLM", "HOLZ", "");
			char* sound = (germanic) ? "S" : "X";
			if (xsConcatenate(&primary, sound, -1) < 0) goto end_free;
			if (xsConcatenate(&secondary, sound, -1) < 0) goto end_free;
			current += 2;
			break;
			}
		    
		    /** Italian & Armenian. **/
		    if (meta_i_isStrAt(&original, current, "SIO", "SIA", "SIAN", ""))
			{
			if (xsConcatenateLiteral(&primary, "S") < 0) goto end_free;
			if (xsConcatenate(&secondary, (is_slavo_germanic) ? "S" : "X", -1) < 0) goto end_free;
			current += 3;
			break;
			}
		    
		    /** german & anglicisations, e.g. 'smith' match 'schmidt', 'snider' match 'schneider' **/
		    /** also, -sz- in slavic language although in hungarian it is pronounced 's' **/
		    if (current == 0 && meta_i_isStrAt(&original, (current + 1), "M", "N", "L", "W", ""))
			{
			if (xsConcatenateLiteral(&primary, "S") < 0) goto end_free;
			if (xsConcatenateLiteral(&secondary, "X") < 0) goto end_free;
			current += 1;
			break;
			}
		    if (meta_i_isStrAt(&original, (current + 1), "Z", ""))
			{
			if (xsConcatenateLiteral(&primary, "S") < 0) goto end_free;
			if (xsConcatenateLiteral(&secondary, "X") < 0) goto end_free;
			current += 2;
			break;
			}
		    
		    if (meta_i_isStrAt(&original, current, "SC", ""))
			{
			/** Schlesinger's rule. **/
			if (meta_i_getCharAt(&original, current + 2) == 'H')
			    {
			    /** Dutch origin, e.g. 'school', 'schooner' **/
			    if (meta_i_isStrAt(&original, (current + 3), "OO", "ER", "EN", "UY", "ED", "EM", ""))
				{
				/** 'schermerhorn', 'schenker' **/
				const bool x_sound = meta_i_isStrAt(&original, (current + 3), "ER", "EN", "");
				if (xsConcatenate(&primary, (x_sound) ? "X" : "SK", -1) < 0) goto end_free;
				if (xsConcatenateLiteral(&secondary, "SK") < 0) goto end_free;
				current += 3;
				break;
				}
			    else
				{
				const bool s_sound = (
				    current == 0
				    && !meta_i_isVowel(&original, 3)
				    && meta_i_getCharAt(&original, 3) != 'W'
				);
				if (xsConcatenateLiteral(&primary, "X") < 0) goto end_free;
				if (xsConcatenate(&secondary, (s_sound) ? "S" : "X", -1) < 0) goto end_free;
				current += 3;
				break;
				}
			    }
			
			/** Default case. **/
			char* sound = (meta_i_isStrAt(&original, (current + 2), "E", "I", "Y", "")) ? "S" : "SK";
			if (xsConcatenate(&primary, sound, -1) < 0) goto end_free;
			if (xsConcatenate(&secondary, sound, -1) < 0) goto end_free;
			current += 3;
			break;
			}
		    
		    /** French e.g. 'resnais', 'artois' **/
		    const bool no_primary = (current == last && meta_i_isStrAt(&original, (current - 2), "AI", "OI", ""));
		    if (xsConcatenate(&primary, (no_primary) ? "" : "S", -1) < 0) goto end_free;
		    if (xsConcatenateLiteral(&secondary, "S") < 0) goto end_free;
		    current += (meta_i_isStrAt(&original, (current + 1), "S", "Z", "")) ? 2 : 1;
		    break;
		    }
		
		case 'T':
		    {
		    if (meta_i_isStrAt(&original, current, "TIA", "TCH", "TION", ""))
			{
			if (xsConcatenateLiteral(&primary, "X") < 0) goto end_free;
			if (xsConcatenateLiteral(&secondary, "X") < 0) goto end_free;
			current += 3;
			break;
			}
		    
		    if (meta_i_isStrAt(&original, current, "TH", "TTH", ""))
			{
			/** Special case 'thomas', 'thames' or germanic. **/
			char* primary_char = (
			    meta_i_isStrAt(&original, (current + 2), "OM", "AM", "")
			    || meta_i_isStrAt(&original, 0, "SCH", "VAN ", "VON ", "")
			) ? "T" : "0"; /* Zero, not O. */
			
			if (xsConcatenate(&primary, primary_char, -1) < 0) goto end_free;
			if (xsConcatenateLiteral(&secondary, "T") < 0) goto end_free;
			current += 2;
			break;
			}
		    
		    if (xsConcatenateLiteral(&primary, "T") < 0) goto end_free;
		    if (xsConcatenateLiteral(&secondary, "T") < 0) goto end_free;
		    current += (meta_i_isStrAt(&original, (current + 1), "T", "D", "")) ? 2 : 1;
		    break;
		    }
		
		case 'V':
		    {
		    if (xsConcatenateLiteral(&primary, "F") < 0) goto end_free;
		    if (xsConcatenateLiteral(&secondary, "F") < 0) goto end_free;
		    current += (next_char == 'V') ? 2 : 1;
		    break;
		    }
		
		case 'W':
		    {
		    /** Can also be in middle of word. **/
		    if (meta_i_isStrAt(&original, current, "WR", ""))
			{
			if (xsConcatenateLiteral(&primary, "R") < 0) goto end_free;
			if (xsConcatenateLiteral(&secondary, "R") < 0) goto end_free;
			current += 2;
			break;
			}
		    
		    const bool next_is_vowel = meta_i_isVowel(&original, current + 1);
		    if (current == 0 && (next_is_vowel || meta_i_isStrAt(&original, current, "WH", "")))
			{
			/** Wasserman should match Vasserman. **/
			if (xsConcatenateLiteral(&primary, "A") < 0) goto end_free;
			if (xsConcatenate(&secondary, (next_is_vowel) ? "F" : "A", -1) < 0) goto end_free;
			}
		    
		    /** Arnow should match Arnoff. **/
		    if ((current == last && meta_i_isVowel(&original, current - 1))
			|| meta_i_isStrAt(&original, (current - 1), "EWSKI", "EWSKY", "OWSKI", "OWSKY", "")
			|| meta_i_isStrAt(&original, 0, "SCH", "")
		       )
			{
			if (xsConcatenateLiteral(&primary, "") < 0) goto end_free;
			if (xsConcatenateLiteral(&secondary, "F") < 0) goto end_free;
			current += 1;
			break;
			}
		    
		    /** Polish e.g. 'filipowicz' **/
		    if (meta_i_isStrAt(&original, current, "WICZ", "WITZ", ""))
			{
			if (xsConcatenateLiteral(&primary, "TS") < 0) goto end_free;
			if (xsConcatenateLiteral(&secondary, "FX") < 0) goto end_free;
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
			    meta_i_isStrAt(&original, (current - 2), "AU", "OU", "")
			    || meta_i_isStrAt(&original, (current - 3), "IAU", "EAU", "")
			)
		    );
		    if (!silent)
			{
			if (xsConcatenateLiteral(&primary, "KS") < 0) goto end_free;
			if (xsConcatenateLiteral(&secondary, "KS") < 0) goto end_free;
			}
		    
		    current += (meta_i_isStrAt(&original, (current + 1), "C", "X", "")) ? 2 : 1;
		    break;
		    }
		
		case 'Z':
		    {
		    /** Chinese pinyin e.g. 'zhao' **/
		    if (next_char == 'H')
			{
			if (xsConcatenateLiteral(&primary, "J") < 0) goto end_free;
			if (xsConcatenateLiteral(&secondary, "J") < 0) goto end_free;
			current += 2;
			break;
			}
		    
		    const bool has_t_sound = (
			meta_i_isStrAt(&original, (current + 1), "ZO", "ZI", "ZA", "")
			|| (is_slavo_germanic && current > 0 && meta_i_getCharAt(&original, (current - 1)) != 'T')
		    );
		    if (xsConcatenateLiteral(&primary, "S") < 0) goto end_free;
		    if (xsConcatenate(&secondary, (has_t_sound) ? "TS" : "S", -1) < 0) goto end_free;
		    current += (next_char == 'Z') ? 2 : 1;
		    break;
		    }
		
		default:
		    current += 1;
		}
	    }
	
	/** Get the output strings. **/
	char* primary_str = xsString(&primary);
	char* secondary_str = xsString(&secondary);
	if (UNLIKELY(primary_str == NULL || secondary_str == NULL)) goto end_free;
	
	/** Allocate buffers for returning the output strings. **/
	void* primary_code_buf = nmSysStrdup(primary_str);
	void* secondary_code_buf = nmSysStrdup(secondary_str);
	if (UNLIKELY(primary_code_buf == NULL || secondary_code_buf == NULL))
	    {
	    if (primary_code_buf != NULL) nmSysFree(primary_code_buf);
	    if (secondary_code_buf != NULL) nmSysFree(secondary_code_buf);
	    goto end_free;
	    }
	
	/** Success. **/
	*primary_code = primary_code_buf;
	*secondary_code = secondary_code_buf;
	ret = 0;
	
    end_free:
	if (UNLIKELY(ret != 0)) fprintf(stderr, "Error: metaDoubleMetaphone() failed (error code %d).\n", ret);
	if (secondary.String != NULL) warnFail(xsDeInit(&secondary));
	if (primary.String != NULL) warnFail(xsDeInit(&primary));
	if (original.String != NULL) warnFail(xsDeInit(&original));
	
	return ret;
    }
