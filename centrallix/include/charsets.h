#ifndef _CHARSETS_H
#define _CHARSETS_H

#include <stddef.h>

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
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
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  		*/
/* 02111-1307  USA							*/
/*									*/
/* A copy of the GNU General Public License has been included in this	*/
/* distribution in the file "COPYING".					*/
/* 									*/
/* Module: 	charsets.c, charsets.h                                  */
/* Author:	Daniel Rothfus                                          */
/* Creation:	June 29, 2011                                           */
/* Description: This module provides utilities for working with the     */
/*              variable character set support built into Centrallix.   */
/************************************************************************/

/** \defgroup charset Character Set Utilities
 \brief This module provides utilities for handling the different character
 set support implemented in Centrallix.
 /{ */

/** \defgroup charset_lookups Looking Up Equivalent Character Set Names
 \brief This contains a collection of macros that define string constants that 
 are to be valid lookup keys in charsetmap.cfg and a function to look up their
 equivalent values.
 \{ */

/** \brief The string constant for the HTTP generation module. */
#define CHR_MODULE_HTTP "http"
/** \brief The string constant for the MySQL database driver. */
#define CHR_MODULE_MYSQL "mysql"
/** \brief A string constant for the Sybase database driver. */
#define CHR_MODULE_SYBASE "sybase"

/** \brief This is the key in centrallix.conf that specifies the charsetmap.cfg
 file */
#define CHR_CHARSETMAP_FILE_KEY "charsetmap_file"

/** \brief Check for an equivalent name for the current character set.
 
 (See charsetmap.cfg for an introduction to equivalent names.)
 \param name The name of the module/subsystem/library that needs a name for the
 current character set.
 \return This returns a pointer to the name of the charset.  This should never
 return NULL, though it may return an incorrect string if the application is 
 configured incorrectly. */
char* chrGetEquivalentName(char* name);

/** \}
 \defgroup charset_stringfuncs String Functions
 \brief These functions operate on multibyte-character strings.  (Or single
 byte, but they are not as efficient as possible with those.)
 
 This is a brief note for the calling convention of all of these functions:
 If a function returns a size_t, it will either return a valid result or one of
 the CHR_* macros on failure.
 If a function takes the arguments buffer and bufferLength, these functions will
 return a pointer to the resulting character string.  The return of these
 functions will either be a valid character string or NULL on error.  If there 
 was an error, the bufferLength will be set to the corresponding CHR_* macro 
 describing the error/problem.  If a non-NULL value was returned, the origin of
 the buffer can be found by examining the pointer returned and the bufferLength
 value after the function is done.  If the bufferLength is 0 and the pointer 
 returned is equal to buffer, the given preallocated buffer was used for the
 results.  If the bufferLength is 0 and the returned pointer is not buffer, then
 the pointer points to some place in the original string, which is only done in 
 certain circumstances.  Otherwise, if the bufferSize is non-zero, the function
 had to allocate memory on the heap with nmSysMalloc, so the returned string
 will need to be freed.
 \todo Make a these functions all work with single byte encodings quickly as
 as well?   (Add special cases for single byte strings?)
 \{ */

/** \brief This constant is returned when an invalid UTF-8 character is found 
 for functions that return type size_t. 
 
 This will also be returned if there was a character set conversion error. */
#define CHR_INVALID_CHAR (size_t)-2
/** \brief This constant is returned for searching functions when the desired
 character is not found in the chr module. */
#define CHR_NOT_FOUND (size_t)-1
/** \brief This is returned when an invalid argument is passed to a function in
 the chr module.  
 
 This is used for such things as NULL pointers with required arguments. */
#define CHR_INVALID_ARGUMENT (size_t)-3
/** \brief Returned when there is not enough memory to do an operation. */
#define CHR_MEMORY_OUT (size_t)-4
/** \brief This is passed when a disallowed (not invalid) character is found
 in a string. */
#define CHR_BAD_CHAR (size_t)-5

/** \brief Get the numeric value of a character (possibly multi-byte).
 \param character A pointer to the character to analyze. 
 \return This returns the numeric value of the first character in the string in
 character based on the current locale.  This could also return an error, such
 as CHR_NOT_FOUND or CHR_INVALID_CHAR.  (See the extended module description for
 more information.) */
size_t chrGetCharNumber(char* character);

/** \brief Get a substring of a given string.
 \param string The (possibly multibyte) string to find the substring in.
 \param begin The character index of the first CHARACTER to start copying at.
 \param end The character index of the character (not byte) right after the
 substring to find.
 \param buffer A preallocated buffer that you want this function to use or NULL.
 \param bufferLength The length of the preallocated buffer.  This must never
 be NULL even if buffer is!
 \return This returns the substring or NULL on error.  (See the extended 
 module description for more information.) */
char* chrSubstring(char* string, size_t begin, size_t end, char* buffer, size_t* bufferLength);

/** \brief Find the first instance of a substring in string.
 \param string The string to find the substring in.
 \param substring The substring to find in the string.
 \return Thist returns the offset into the string that the character was found
 at or an error, such as CHR_NOT_FOUND or CHR_INVALID_CHAR. (See the extended 
 module description for more information.) */
size_t chrSubstringIndex(char* string, char* substring);

/** \brief Convert a (possibly multibyte) string to upper case.
 \param string The string to convert to upper case.
 \param buffer A preallocated buffer that you want this function to use or NULL.
 \param bufferLength The length of the preallocated buffer.  This must never
 be NULL even if buffer is!
 \return This returns the upper case string or NULL on error. (See the extended 
 module description for more information.) */
char* chrToUpper(char* string, char* buffer, size_t* bufferLength);

/** \brief Convert a (possibly multibyte) string to lower case.
 \param string The string to convert to lower case.
 \param buffer A preallocated buffer that you want this function to use or NULL.
 \param bufferLength The length of the preallocated buffer.  This must never
 be NULL even if buffer is!
 \return This returns the lower case string or NULL on error. (See the extended 
 module description for more information.) */
char* chrToLower(char* string, char* buffer, size_t* bufferLength);

/** \brief This function returns the number of characters (NOT BYTES) in a 
 given string.
 \param string The string to return the number of characters in. 
 \return This returns the length or an error such (but not limited to) as 
 CHR_INVALID_CHAR on error. */
size_t chrCharLength(char* string);

/** \brief This function ensures that a multibyte string will be in simplest form.
 \param string The string to simplify.
 \return This returns the same string but with the minimum amount of bytes used
 or NULL on error. */
char* chrNoOverlong(char* string);

/** \brief Get a specific number of the rightmost characters in the string.
 
 This function always returns an offset into the original string since no
 copying is ever needed.
 \param string The string to get numChars number of rightmost characters.
 \param numChars The number of characters to grab off of the right of the
 string.
 \param returnCode This takes the role of bufferLength in the other char*
 returning functions for return codes.
 \return This returns a pointer to the position in the original string where
 only the numChar(th) right characters are viewable from.
 */
char* chrRight(char* string, size_t numChars, size_t* returnCode);

/** \brief Escape certain characters in a string with '\' while banning some.
 \param string The string to escape.
 \param escape A string with all of the characters to escape.
 \param bad The bad characters to stop processing on.  When it stops processing,
 it returns NULL and sets the value of bufferLength to CHR_BAD_CHAR.
 \param buffer A preallocated buffer that you want this function to use or NULL.
 \param bufferLength The length of the preallocated buffer.  This must never
 be NULL even if buffer is!
 \return This returns the escaped string which may point to the preallocated 
 buffer or an allocated buffer or NULL if a bad character was encountered. (See
 the extended module description for more information.) */
char* chrEscape(char* string, char* escape, char* bad, char* buffer, size_t* bufferLength);

/** \brief Create a string of a certain minimum size with spaces padding the 
 string to the right if the string is not long enough.
 \param string The string to resize to the minimum given length.
 \param minLength The minimum number of characters that you want the string to
 take up.
 \param buffer A preallocated buffer that you want this function to use or NULL.
 \param bufferLength The length of the preallocated buffer.  This must never
 be NULL even if buffer is!
 \return This returns the buffered string or NULL on error.  Please note  that
 this is extremely prone to returning the original string because this is done
 whenever the string is the same length or longer than the minLength.  (See the
 extended module description for more information.)
 */
char* chrRightAlign(char* string, size_t minLength, char* buffer, size_t* bufferLength);

/** \brief Convenience function for checking the validity of results.
 \param result The result value obtained from the string function.
 \return Returns 1 if valid, 0 if not valid.
 */
int chrValid(int result);

/** \} */

/** \} */

#endif
