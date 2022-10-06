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

/**
 * Validates a utf8 string and ensures no invalid characters are present. 
 * @param str the string to be verified 
 * @return the index of first byte of the first invalid character, or 
 *   UTIL_VALID_CHAR or UTIL_INVALID_ARGUMENT when applicable 
 */
int verifyUTF8(char* str)
	{
    typedef unsigned char CHAR;
	int state = UTIL_STATE_START;
    int off = 0;
    int lastStart = -1; /** need start of invalid chars **/
    /** Check arguments **/
	if(!str)
        return UTIL_INVALID_ARGUMENT;
    
    while(str[off] != '\0')
        {
        switch(state)
            {
            case UTIL_STATE_START:
                if((CHAR) str[off] <= (CHAR) 0x7F) state = UTIL_STATE_START;        /** of the form 0XXX XXXX **/
                else if ((CHAR) str[off] <= (CHAR) 0xBF) state = UTIL_STATE_ERROR;  /** not a header **/
                else if ((CHAR) str[off] <= (CHAR) 0xC1) state = UTIL_STATE_ERROR;  /** overlong form **/
                else if ((CHAR) str[off] <= (CHAR) 0xDF) state = UTIL_STATE_1_BYTE; /** of the form 110X XXXX **/
                else if ((CHAR) str[off] == (CHAR) 0xE0) state = UTIL_STATE_E_OVERLONG; /** check for overlong **/
                else if ((CHAR) str[off] == (CHAR) 0xED) state = UTIL_STATE_E_SURROGATE;
                else if ((CHAR) str[off] <= (CHAR) 0xEF) state = UTIL_STATE_2_BYTE; /** of the form 1110 XXXX **/
                else if ((CHAR) str[off] == (CHAR) 0xF0) state = UTIL_STATE_F_OVERLONG; /** check for overlong **/
                else if ((CHAR) str[off] <= (CHAR) 0xF3) state = UTIL_STATE_3_BYTE; 
                else if ((CHAR) str[off] == (CHAR) 0xF4) state = UTIL_STATE_TOO_LARGE;
                else state = UTIL_STATE_ERROR; /** header must be too big **/

                lastStart = off;
                if(state != UTIL_STATE_ERROR) off++;
                break;

            case UTIL_STATE_3_BYTE:
                if((CHAR) str[off] > (CHAR) 0xBF || (CHAR) str[off] < (CHAR) 0x80) state = UTIL_STATE_ERROR;
                else state = UTIL_STATE_2_BYTE;

                if(state != UTIL_STATE_ERROR) off++;
                break;

            case UTIL_STATE_2_BYTE:
                if((CHAR) str[off] > (CHAR) 0xBF || (CHAR) str[off] < (CHAR) 0x80) state = UTIL_STATE_ERROR;
                else state = UTIL_STATE_1_BYTE;
                
                if(state != UTIL_STATE_ERROR) off++;
                break;

            case UTIL_STATE_1_BYTE:
                if((CHAR) str[off] > (CHAR) 0xBF || (CHAR) str[off] < (CHAR) 0x80) state = UTIL_STATE_ERROR;
                else state = UTIL_STATE_START;

                if(state != UTIL_STATE_ERROR) off++;
                break; 

            case UTIL_STATE_E_OVERLONG:
                if((CHAR) str[off] <= (CHAR) 0x9F ) state = UTIL_STATE_ERROR;
                else state = UTIL_STATE_2_BYTE; /** jump back in **/
                break;
            
            case UTIL_STATE_E_SURROGATE:
                if((CHAR) str[off] >= (CHAR) 0xA0 ) state = UTIL_STATE_ERROR;
                else state = UTIL_STATE_2_BYTE; /** jump back in **/
                break;

            case UTIL_STATE_F_OVERLONG:
                if((CHAR) str[off] <= (CHAR) 0x8F ) state = UTIL_STATE_ERROR;
                else state = UTIL_STATE_3_BYTE; /** jump back in **/
                break;

            case UTIL_STATE_TOO_LARGE: 
                if((CHAR) str[off] > (CHAR) 0x8F) state = UTIL_STATE_ERROR;
                else state = UTIL_STATE_3_BYTE; /** jump back in **/
                break; 

            case UTIL_STATE_ERROR:
                return lastStart;
                break;
            }
        }
    if(state != UTIL_STATE_START) return lastStart; /** end of string splits char **/
  
	return  UTIL_VALID_CHAR;
	}

/**
 * Validates an ASCII string and ensures no invalid characters are present. 
 * @param str the string to be verified 
 * @return the index of first invalid character, or UTIL_VALID_CHAR or 
 * UTIL_INVALID_ARGUMENT when applicable 
 */
int 
verifyASCII(char * str)
    {
    typedef unsigned char CHAR;
    int off = 0;
    if(!str) return UTIL_INVALID_ARGUMENT;
    
    while(str[off] != '\0')
        {
        if((unsigned) str[off] > 127) return off;
        off++;
        }
    return UTIL_VALID_CHAR;
    }
