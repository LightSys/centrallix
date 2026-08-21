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

/** TODO: ISRAEL - Remove these after the dups branch is merged. **/

/*** @brief Returns the smaller of two values.
 *** 
 *** @param a The first value.
 *** @param b The second value.
 *** @return The smaller of the two values.
 *** 
 *** @note This macro uses GCC extensions to ensure type safety.
 ***/
#define min(a, b) \
    ({ \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    (_a < _b) ? _a : _b; \
    })

/*** @brief Returns the larger of two values.
 *** 
 *** @param a The first value.
 *** @param b The second value.
 *** @return The larger of the two values.
 *** 
 *** @note This macro uses GCC extensions to ensure type safety.
 ***/
#define max(a, b) \
    ({ \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    (_a > _b) ? _a : _b; \
    })

#endif	/* UTILITY_H */
