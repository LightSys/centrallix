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
    if(tmp>=INT_MAX){
        errno = ERANGE;
        return INT_MAX;   
    }else if(tmp<=INT_MIN){
        errno = ERANGE;
        return INT_MIN;
    }
    //return as tmp;
    return (int)tmp;
}

/**
 * Converts a string to the unsigned integer value represented
 *  by the string, based on strtoul, but returns error on negative 
 * @param nptr   the string
 * @param endptr if not NULL will be assigned the address of the first invalid character
 * @param base   the base to convert with
 * @return the converted int or UINT_MAX on error (including negative)
 */
unsigned int strtoui(const char *nptr, char **endptr, int base){
    unsigned long tmp;
    //try to convert
    tmp = strtoul(nptr,endptr,base);
    ///FOR DEBUGING!!!!
    //if(strtol(nptr,endptr,base)<0)fprintf(stderr,"@@@%lu@@@\n",tmp);
    //check for errors in conversion to long
    if(tmp == ULONG_MAX){
        return UINT_MAX;
    }
    //now check for error in conversion to int
    if(tmp>=UINT_MAX){
        errno = ERANGE;
        return UINT_MAX;   
    }  
    //return as tmp;
    return (unsigned int)tmp;
}