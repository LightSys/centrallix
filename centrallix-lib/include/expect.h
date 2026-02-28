#ifndef _EXPECT_H
#define _EXPECT_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module:	Expect Branch Optimization Module (mtask.c, mtask.h)	*/
/* Author:	Israel Fuller						*/
/* Date:	February 27th, 2026					*/
/*									*/
/* Description:	EXPECT is a set of optimization macros for signalling	*/
/* 		the more likely branch to the compiler, including	*/
/* 		fallbacks for compatibility with compilers that do not  */
/* 		support these optimization hints.			*/
/************************************************************************/


/*** Define macros for signalling the more likely branch to the compiler.
 *** 
 *** Note: The doc comments are written on the the fallback macros because
 *** this optimization is unlikely to be defined in the intellisense module
 *** for the developer's editor.
 ***/
#ifdef __builtin_expect
/** Use the GCC __builtin_expect() function for optimization. **/
#define LIKELY(x)   __builtin_expect((x), 1)
#define UNLIKELY(x) __builtin_expect((x), 0)
#else
/*** Fallback: Define pass through functions to support compilers that don't
 *** have this feature.
 ***/

/** Indicates that x is likely to be true. **/
#define LIKELY(x)   (x)
/** Indicates that x is likely to be false. **/
#define UNLIKELY(x) (x)
#endif

#endif