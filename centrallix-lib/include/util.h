#ifndef UTILITY_H
#define	UTILITY_H

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

#ifdef	__cplusplus
extern "C" {
#endif


    int strtoi(const char *nptr, char **endptr, int base);
    unsigned int strtoui(const char *nptr, char **endptr, int base);


#ifdef	__cplusplus
}
#endif

#endif	/* UTILITY_H */

#define UTIL_INVALID_CHAR (size_t)-2
#define UTIL_INVALID_ARGUMENT (size_t)-3

/** \brief This function ensures that a multibyte string will be in simplest form.
 \param string The string to simplify.
 \return This returns the same string but with the minimum amount of bytes used
 or NULL on error. */
int verifyUTF8(char* string);

