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

#define UTIL_VALID_CHAR (size_t)-1 /** for use with verifyUTF8 **/
#define UTIL_INVALID_CHAR (size_t)-2
#define UTIL_INVALID_ARGUMENT (size_t)-3
/** states for validate **/
#define UTIL_STATE_START  0
#define UTIL_STATE_3_BYTE 1 /** 3 bytes left; was 4 total **/
#define UTIL_STATE_2_BYTE 2 /** 2 bytes left; a leats 3 total **/
#define UTIL_STATE_1_BYTE 3 /** 1 byte left; was at least 2 total **/
#define UTIL_STATE_ERROR  4
#define UTIL_STATE_E_SURROGATE 5
#define UTIL_STATE_E_OVERLONG 6 /** starts with E0, check for overlong **/
#define UTIL_STATE_F_OVERLONG 7 /** starts with F0, check for overlong **/
#define UTIL_STATE_TOO_LARGE 8  /** starts with F4, check for too long **/

/** \brief This function ensures that a string contains valid UTF-8.
 \param string The string to verify.
 \return The index of the first byte of the first invald char, or a 
  code if not applicable  */
int verifyUTF8(char* str);

/** \brief This function ensures that all of the bytes of a string are 
     valid ASCII.
 \param string The string to verify.
 \return returns index of the first invalid string, or a code if not 
  applicable */
int verifyASCII(char * str);