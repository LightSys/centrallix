#ifndef _MEMSTR_H
#define _MEMSTR_H

#include <stdlib.h>
#include <string.h>

/************************************************************************/
/* Centrallix Application Server System                                 */
/* Centrallix Base Library                                              */
/*                                                                      */
/* Copyright (C) 1998-2014 LightSys Technology Services, Inc.           */
/*                                                                      */
/* You may use these files and this library under the terms of the      */
/* GNU Lesser General Public License, Version 2.1, contained in the     */
/* included file "COPYING".                                             */
/*                                                                      */
/* Module:	memstr.h, memstr.c                                          */
/* Author:	Brady Steed (BLS)                                           */
/* Date:	July 8, 2014                                                */
/*									                                    */
/* Description:	Provides memstr(), a function that searches a buffer	*/
/*		for a string. Similar to strstr, but uses a size instead        */
/*		of null character for a delimiter.                              */
/************************************************************************/


char * memstr(char* buffer, const char* substr, size_t bufferlen);

#endif /* not defined _MEMSTR_H */