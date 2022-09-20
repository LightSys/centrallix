/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 1998-2011 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module:	(util.c,.h)                                             */
/* Author:	Micah Shennum                                           */
/* Date:	May 26, 2011                                            */
/* Description:	Collection of utilities                                 */
/************************************************************************/

#include <stdio.h>

#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include "util.h"

/**
 * Converts a string to the integer value represented
 *  by the string, based on strtol.
 * @param nptr   the string
 * @param endptr if not NULL will be assigned the address of the first invalid character
 * @param base   the base to convert with
 * @return the converted int or INT_MAX/MIN on error
 */
int strtoi(const char *nptr, char **endptr, int base){
    long tmp;
    //try to convert
    tmp = strtol(nptr,endptr,base);
    //check for errors in conversion to long
    if(tmp == LONG_MAX){
        return INT_MAX;
    }
    if(tmp==LONG_MIN){
        return INT_MIN;
    }
    //now check for error in conversion to int
    if(tmp>INT_MAX){
        errno = ERANGE;
        return INT_MAX;   
    }else if(tmp<INT_MIN){
        errno = ERANGE;
        return INT_MIN;
    }
    //return as tmp;
    return (int)tmp;
}

/**
 * Converts a string to the unsigned integer value represented
 *  by the string, based on strtoul
 * @param nptr   the string
 * @param endptr if not NULL will be assigned the address of the first invalid character
 * @param base   the base to convert with
 * @return the converted int or UINT_MAX on error
 */
unsigned int strtoui(const char *nptr, char **endptr, int base){
    long long tmp;
    //try to convert
    tmp = strtoll(nptr,endptr,base);
    
    //check for errors in conversion to long
    if(tmp == ULONG_MAX){
        return UINT_MAX;
    }
    //now check for error in conversion to int
    if(tmp>UINT_MAX){
        errno = ERANGE;
        return UINT_MAX;   
    }  
    //return as tmp;
    return (unsigned int)tmp;
}

/*** verifyUTF8: validates a utf8 string and ensures no invalid characters 
 *** are present. Returns 0 on a valid string, or an error code otherwise
 ***/
int verifyUTF8(char* string)
	{
	size_t stringCharLength, newStrByteLength;
	char* result;
	wchar_t* longBuffer;

        /** Check arguments **/
	if(!string)
        	return UTIL_INVALID_ARGUMENT;

    /* ensure no overly large characters are included */
    int i;
    for(i = 0 ; i < strlen(string) ; i++)
        {
        if((unsigned char) string[i] == (unsigned char) 0xF4)
            {
            /* make sure is less than F4 90 */
            /* this is safe since it would only hit the null byte */
            if((unsigned char) string[i+1] >= (unsigned char)0x90) return UTIL_INVALID_CHAR; 
            }
        /* if true, must be a header for more than 4 bytes */
        else if( (unsigned char) string[i] > (unsigned char) 0xF4) return UTIL_INVALID_CHAR; 
        }
	
	stringCharLength = mbstowcs(NULL, string, 0);
	if(stringCharLength == (size_t)-1)
            	{
        	return UTIL_INVALID_CHAR;
       		}	

	/** Create wchar_t buffer */
        longBuffer = nmSysMalloc(sizeof(wchar_t) * (stringCharLength + 1));
        if(!longBuffer)
        	return UTIL_INVALID_CHAR;
        mbstowcs(longBuffer, string, stringCharLength + 1);	
	
	/** Convert back to MBS **/
	newStrByteLength = wcstombs(NULL, longBuffer, 0);
        if(newStrByteLength == (size_t)-1)
            	{
            	nmSysFree(longBuffer);
        	return UTIL_INVALID_CHAR;
            	}
	
	result = (char *)nmSysMalloc(newStrByteLength + 1);
        if(!result)
            	{
                nmSysFree(longBuffer);
                return UTIL_INVALID_CHAR;
            	}
            
        wcstombs(result, longBuffer, newStrByteLength + 1);
        
	nmSysFree(longBuffer);
    if(strcmp(result, string) != 0 ) return UTIL_INVALID_CHAR; 
    nmSysFree(result); 
	
	return  0;
	}

