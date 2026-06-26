#ifndef RANGE_H
#define	RANGE_H

/************************************************************************/
/* Centrallix Application Server System                                 */
/* Centrallix Base Library                                              */
/*                                                                      */
/* Copyright (C) 1998-2026 LightSys Technology Services, Inc.           */
/*                                                                      */
/* You may use these files and this library under the terms of the      */
/* GNU Lesser General Public License, Version 2.1, contained in the     */
/* included file "COPYING".                                             */
/*                                                                      */
/* Module:      range.c, range.h                                        */
/* Author:      Israel Fuller                                           */
/* Date:        October 13, 2025                                        */
/* Description: Adds some useful numerical range functions/macros that  */
/*              C does not provide by default.                          */
/************************************************************************/

#ifndef min
/*** Returns the smaller of two values.
 *** 
 *** @param a The first value.
 *** @param b The second value.
 *** @return The smaller of the two values.
 ***/
#define min(a, b) \
    ({ \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    (_a < _b) ? _a : _b; \
    })
#endif

#ifndef max
/*** Returns the larger of two values.
 *** 
 *** @param a The first value.
 *** @param b The second value.
 *** @return The larger of the two values.
 ***/
#define max(a, b) \
    ({ \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    (_a > _b) ? _a : _b; \
    })
#endif

/*** Clamps the value between a minimum and maximum bound.
 *** 
 *** If minimum > maximum, always returns maximum.
 *** 
 *** @param minimum The smallest allowed value.
 *** @param value   The value to clamp.
 *** @param maximum The largest allowed value.
 *** @return The value clamped within the range [minimum, maximum].
 ***/
#define clamp(minimum, value, maximum) \
    max(minimum, min(value, maximum))

#endif	/* RANGE_H */
