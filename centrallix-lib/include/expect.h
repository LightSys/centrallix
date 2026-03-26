#ifndef _EXPECT_H
#define _EXPECT_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 1998-2026 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module:	Expect Branch Optimization Module (expect.h)		*/
/* Author:	Israel Fuller						*/
/* Date:	February 27th, 2026					*/
/*									*/
/* Description:	EXPECT is a set of optimization macros for signalling	*/
/* 		the more likely branch to the compiler, including	*/
/* 		fallbacks for compatibility with compilers that do not  */
/* 		support these optimization hints.			*/
/* 		LIKELY() indicates the passed boolean is likely to be	*/
/* 		true, UNLIKELY() indicates it is likely to be false.	*/
/************************************************************************/


/** Define macros for signalling the more likely branch to the compiler. **/
#ifdef HAVE_BUILTIN_EXPECT
/** Use the GCC __builtin_expect() function for optimization. **/
/** Note: We use !!(x) which normalizes to 0 or 1 to help the compiler. **/
#define LIKELY(x)   (__builtin_expect(!!(x), 1))
#define UNLIKELY(x) (__builtin_expect(!!(x), 0))
#else
/*** Fallback: Define pass through functions to support compilers that don't
 *** have this feature.
 ***/
#define LIKELY(x)   (x)
#define UNLIKELY(x) (x)
#endif

#endif
